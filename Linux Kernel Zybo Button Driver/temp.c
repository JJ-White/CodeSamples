This driver creates multiple files in sysfs and devfs to control button, led, and switches on the Zybo board.

In sysfs the directory '/sys/kernel/btnled/' is created containing file 'conf'. This file can be used to read button/switch states or set leds. It expects the following arguments:

    c i i
    c - 'b'/'s'/'l' depending on the device being controlled
    i - Device index, e.g. 2 for led2
    i - In case of led, the state to set 0 for off, 1 for on
    
In devfs multiple files are created when running the script 'create_nodes.sh', these can later be removed using 'remove_nodes.sh'. The following files are created:

    led0 - control point for led, read for state, write 0/1 to set state
    led1 - control point for led, read for state, write 0/1 to set state
    led2 - control point for led, read for state, write 0/1 to set state
    led3 - control point for led, read for state, write 0/1 to set state
    button0 - control point for button, read for button state
    button1 - control point for button, read for button state
    button2 - control point for button, read for button state
    button3 - control point for button, read for button state
    switch0 - control point for switch, read for switch state
    switch1 - control point for switch, read for switch state
    switch2 - control point for switch, read for switch state
    switch3 - control point for switch, read for switch state
    
Known bugs:
    
    - Only one dev file can be open at a time due to one devOpen being used
    - register_chrdev throws error EBUSY when registering multiple files, button and switch 
      files have been disabled
