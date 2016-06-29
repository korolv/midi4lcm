# midi4lcm
This is simple server to translate I-bus LCM control messages from  midi event messages. It opens multicast UDP port to receive MIDI, looks for note on/off messages and translates them to I-bus messages.

| Light unit                     | Note |
|--------------------------------|------|
| fog light front right          | 24   |
| turn signal front right        | 25   |
| parking light front right      | 26   |
| low beam light front right     | 27   |
| high beam light front right    | 28   |
| high beam light front left     | 29   |
| low beam light front left      | 30   |
| parking light front left       | 31   |
| turn signal front left         | 32   |
| fog light front left           | 33   |
| brake light back right         | 34   |
| backup light back right        | 35   |
| fog light back right           | 36   |
| tail light outside back right  | 37   |
| tail light inside back right   | 38   |
| turn signal back right         | 39   |
| registration plate light right | 40   |
| brake light back center        | 41   |
| registration plate light left  | 42   |
| turn signal back left          | 43   |
| tail light inside back left    | 44   |
| tail light outside back left   | 45   |
| fog light back left            | 46   |
| backup light back left         | 47   |
| brake light back left          | 48   |
