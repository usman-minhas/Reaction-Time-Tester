# Reaction-Time-Tester
Coded a reaction time tester in C using the STM32F microcontroller. Used **loops, functions, interrupts, external interrupts, and extern EEPROM**.

## Logic
Upon reset the display shows the highest score set and flashes the LEDs at 1 Hz.

The player can then click the user button which powers off the LEDs for a randomized time. Once the LEDs turn on, the player can press the user button to record their time (Times out after 10 seconds).

If they press the button prior to the LEDs turning on or exactly at the time they turn on, a message saying "Cheater" will be displayed.

The display will show the current score and the highest score. If the player sets a new high score, the time will be stored in an external EEPROM.
