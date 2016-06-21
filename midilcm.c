/*
 * Copyright 2016 Vladimir Korol <vovabox@mail.ru>
 *
 * midi4lcm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * midi4lcm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with midi4lcm. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/poll.h>
#include <fcntl.h>
#include "ikbus.h"
#include "dialcm.h"

#define MIDI_UDP_PORT 21928
#define MIDI_BUF_SIZE 256
#define MIDI_NOTE_OFFSET 24
#define MIDI_INTEGRATE_DELAY 1 // ms

typedef enum {
    STATUS,
    DATA1,
    DATA2,
    OTHER
} midi_byte_t;

unsigned char midi_udp_buffer[MIDI_BUF_SIZE];
unsigned char midi_ibus_buffer[24] = {0};
int udp_port = MIDI_UDP_PORT;
in_addr_t ip_interface = 0;
int ibus_socket;
int running = 0;

static const char validargs[] =
"\nValid Arguments:\n"
"\t--interface <XXX.XXX.XXX.XXX>  IP address of network interface\n"
"\t--udp <XXXXX>                   UDP port 21928-21947\n";


/* Parse input arguments */
int parseargs(int argc, char **argv)
{
    while(--argc) {
        argv++;
        /* UDP port */
        if(strcasecmp(*argv, "--udp")==0) {
            unsigned long int temp;
            if(--argc < 1)
                return -1;

            argv++;
            if(!isdigit(**argv))
                return -1;

            temp = strtoul(*argv, NULL, 10);
            if((temp >= 21928) && (temp <= 21947)) {
                udp_port = (unsigned int) temp;
            } else
                return -1;
        }

        /* Net interface */
        else if(strcasecmp(*argv, "--interface")==0) {
            if(--argc < 1)
                return -1;

            argv++;

            ip_interface = inet_addr(*argv);
            if(ip_interface == -1) {
                ip_interface = 0;
                return -1;
            }
        }
        /* Invalid argument */
        else
            return -1;
    }

    return 0;
}

int help(char *argv0, FILE *out)
{
    fprintf(out, "usage: %s [arguments]\n%s\n", argv0, validargs);

    return 0;
}

light_mask_t midi_to_light(int note)
{
    int num = note - MIDI_NOTE_OFFSET;
    light_mask_t light_set = 0;

    if ((num >= 0) && (num < LIGHT_UNIT_SIZE)) {
        light_set = light_units[num].bitmask;
    }

    return light_set;
}

int main(int argc, char** argv)
{
    int udp_socket;
    u_int yes = 1;
    struct sockaddr_in uaddr;
    struct ip_mreq mreq;
    struct sockaddr_ikbus iaddr;
    struct ifreq ifr;
    char *ifname = "ibus0";
    light_mask_t *light;
    struct pollfd fd;
    int timeout;

    if(parseargs(argc, argv) < 0) {
        help(argv[0], stderr);
        return 0;
    }

    /* Bind UDP socket */
    udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if(udp_socket < 0) {
        perror("socket");
        return (-1);
    }

    /* allow multiple sockets to use the same PORT number */
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt multiple sockets");
        return (-1);
    }

    uaddr.sin_family = AF_INET;
    uaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    uaddr.sin_port = htons(udp_port);
    if(bind(udp_socket, (struct sockaddr *) &uaddr, sizeof(uaddr))<0) {
        perror("bind");
        return (-1);
    }

    /* use setsockopt() to request that the kernel join a multicast group */
    mreq.imr_multiaddr.s_addr = inet_addr("225.0.0.37");
    mreq.imr_interface.s_addr = ip_interface;
    if (setsockopt(udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt mreq");
        return (-1);
    }

    /* Set up poll */
    if (fcntl(udp_socket, F_SETFL, O_NONBLOCK)) {
        perror("fcntl nonblock");
        return (-1);
    }
    fd.fd = udp_socket;
    fd.events = POLLIN;
    timeout = -1;

     /* I/K-bus socket */
    ibus_socket = socket(PF_IKBUS, SOCK_RAW, 0);
    if(ibus_socket < 0) {
        perror("socket");
        return (-1);
    }

    strcpy(ifr.ifr_name, ifname);
    ioctl(ibus_socket, SIOCGIFINDEX, &ifr);
    iaddr.ikbus_family  = AF_IKBUS;
    iaddr.ifindex = ifr.ifr_ifindex;

    /* Bind I/K-bus socket */
    if(bind(ibus_socket, (struct sockaddr *) &iaddr, sizeof(iaddr)) < 0) {
        perror("bind");
        return -1;
    }

    running = 1;

    /* Initialization I/K-bus TX buffer */
    midi_ibus_buffer[IKBUS_FRM_SENDER] = IKBUS_DEV_DIA;
    midi_ibus_buffer[IKBUS_FRM_SIZE] = 15;
    midi_ibus_buffer[IKBUS_FRM_RECEIVER] = IKBUS_DEV_LCM;
    midi_ibus_buffer[IKBUS_FRM_CMD] = IKBUS_MSG_VEHICLE_CTL;
    light = (light_mask_t *) (midi_ibus_buffer + IKBUS_FRM_DATA + 4);

    while(running) {
        int len, note, i, res;
        midi_byte_t type;
        unsigned char on_off;

        res = poll(&fd, 1, timeout);
        if ((res < 0) && running) {
            perror("poll");
            break;
        }

        if (res == 0) { /* timeout */
            /* Send integrated message to I/K-bus */
            int size;
            size = (int) midi_ibus_buffer[IKBUS_FRM_SIZE] + 1;
            write(ibus_socket, midi_ibus_buffer, size);
            timeout = -1;
            continue;
        }

        len = read(udp_socket, midi_udp_buffer, MIDI_BUF_SIZE);

        type = OTHER;
        /* Parsing midi data... */
        for (i = 0; i < len; i++) {
            /* Filter midi events from first cannel */
            if ((midi_udp_buffer[i] == 0x90) || (midi_udp_buffer[i] == 0x80)) {
                type = STATUS;
                on_off = ((midi_udp_buffer[i] & 0x10) == 0x10);
            }
            else if (type == STATUS) {
                /* Data byte */
                type = DATA1;
                note = midi_udp_buffer[i];
                if (on_off)
                    *light |= midi_to_light(note); /* swith on */
                else
                    *light &= ~midi_to_light(note); /* switch off */
            }
            /* else ... */
        }
        if (type == DATA1) {
            /* Set delay to integrate midi events */
            timeout = MIDI_INTEGRATE_DELAY;
            type = OTHER;
        }
    }

    return 0;
}
