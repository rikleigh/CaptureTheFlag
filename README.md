# Capture The Flag for drones
![Alt text](pictures/CTF.jpg?raw=true "CTF")

// Current version: 0.3 alpha //


I would like to share something I’ve been wanting for a while so I just decided to learn Arduino and do it myself.

**It’s Capture The Flag for your drones (with FPV).**

See explanation video here: https://www.youtube.com/watch?v=plKc7N_GApc

If you happen to stumble here and don't know what FPV you can read about it in [Wikipedia](https://en.wikipedia.org/wiki/First-person_view_(radio_control)) or simply visit my youtube channel *www.youtube.com/SeekNDFPV*



This mod was made with Tiny Whoops (check *TinyWhoop.com*) in mind but really you can use it with any size drone as long as it has a 5.8 Ghz video transmitter on it.

It’s fairly easy to build, **costs around £9 (or $12) per flag in materials** and you just need to solder a few cables 🙂

You can battle **4 drones (BLUE TEAM - RACEBAND 1 to 4) vs 4 drones (RED TEAM - RACEBAND 5 to 8)** in a game of Capture The Flag (as long as your VTX signals allow for that many to be flying at the same time) with as many flags as you are bothered to build!
There are also two new modes you can now try out.

The code itself is GREATELY based off Chorus RF Laptimer
Visit their repo if you are interested in knowing more about it here: https://github.com/voroshkov/Chorus-RF-Laptimer

Unlike Chorus RF Laptimer, at this moment this mod is standalone and cannot be accessed remotely (yet, maybe someone would like to help code that?)

You can though, change the code to match your needs.
I've commented everything that I've changed in the code with (CC) and the explanation behind it so it's easy for you to find it and replace it. Pretty sure the code can be improved by A LOT but hey .. it's working!

## Materials you need

> 1x Arduino Nano (or an uno if you have one should also work)
https://www.banggood.com/ATmega328P-Arduino-Compatible-Nano-V3-Improved-Version-No-Cable-p-959231.html

> 1x FPV Receiver module with SPI mod (these below already have the mod but double check - google vtx spi mod or check the pictures in this repo)
https://www.banggood.com/FPV-5_8G-Wireless-Audio-Video-Receiving-Module-RX5808-p-84775.html


> 1x WS2812 led strip (I used the one below - do not recommend using a bigger strip then 10 leds without external power)
https://www.banggood.com/Eachine-Aurora-90-100-Mini-FPV-Racer-Spare-Part-WS2812-LED-Board-LED-Strip-Light-p-1122903.html

> The hability to solder 9 cables :)  ( oh .. you also need 9 cables :P )

 
## How to assemble it

1. Start by soldering the WS2812 strip Ground cable (black) to Arduino ground, the WS2812 strip 5V cable (red) to Arduino 5V and the WS2812 strip din cable (blue) to Arduino D5
2. Now solder the Channel 1 pin in your VTX receiver to your Arduino D10
3. Solder the Channel 2 pin in your VTX receiver to your Arduino D11
4. Solder the Channel 3 pin in your VTX receiver to your Arduino D12
5. Solder the Ground pin in your VTX receiver to your Arduino Ground (the same ground you connected the WS2812 ground to)
6. Solder the 5v pin in your VTX receiver to your Arduino 5V (the same 5v you connected the WS2812 5v to)
7. And finally, solder the RSSI pin in your VTX receiver to your Arduino A3

[Picture 1 example](pictures/IMG_1432.JPG)

[Picture 2 example](pictures/IMG_1433.JPG)

[Picture 3 example](pictures/IMG_1434.JPG)

What? That's it?
Almost! You just need to upload the code to your Arduino via usb and you are done.


## How to upload the code

1. Start by downloading and installing Arduino IDE (https://www.arduino.cc/en/Main/Software)
2. Next, download THIS repository and unzip it somewhere in your desktop.
3. Go to the new folder you downloaded and open the file called CTF.INO
4. You will now be able to see a bunch of code and several files open.
5. Goto Tools/Board and pick Arduino Nano (if you bought an arduino nano, otherwise pick the one you have). The processor is ATMega328P.
6. Now to go port and pick the port your Arduino Nano is connected to. If you don't know which one it is, disconnect the nano and check what ports are available. Reconnect the nano, the new port will be the nano. If you don't get any new ports I'm afraid you will need to google how to troubleshoot it :)
7. VERY IMPORTANT. You will need to play with the RSSITHRESHOLD value to find what works best for you. I find that most receivers work differently and that value can be between 190 to 260. Test it out with with EACH nano you build
8. You can also change MODES (view further down) by changing the MODE value in the code.
9.Finally, when you are ready, goto sketch menu and select upload. If you done everything right you will see an increasing bar on the bottom right handside of the program and then a message saying Done uploading on the lower left handside.




# Congratulations!
You are now the proud owner of a CTF flag!

1. The arduino board can be powered by connecting 5v to the 5v connection OR by connecting it to a power bank via USB.
2. You can now also 3D print a neat cover for the arduino : https://www.thingiverse.com/thing:2766437
3. Once you power up the arduino board it will start scanning for all RACEBAND frequencies. It takes ~6 seconds to do a complete loop. 
4. Once it finds the first frequency it will change the LED colour depending if you are BLUE TEAM (RACEBAND 1-4) or RED TEAM (RACEBAND 5-8)
5. Once the flag changes colour it will only scan for the other team frequencies so the scanning takes place faster (~3 seconds for a loop)
6. Protect your flags anyway you can!

## MODES

There are currently 3 modes available

1. Capture the flag. Your standard capture the flag where two teams battle it out to have their flag and their opponents flag captured
- R1-R4: TEAM BLUE    //     R5-8: TEAM BLUE

2. Free For All Deathmatch. You are on your own and need to capture as many flags for yourself as you can.
- R1 BLUE
- R2 GREEN
- R3 YELLOW
- R4 RED
- R5 CYAN
- R6 VIOLET
- R7 WHITE
- R8 // not used at the moment

3. Scavenger Hunt. All flags have been hidden. It's your mission to find as many as you can before your battery ends.
- R1 BLUE
- R2 GREEN
- R3 YELLOW
- R4 RED
- R5 CYAN
- R6 VIOLET
- R7 WHITE
- R8 // not used at the moment

### GAME IDEA 1 (2 flags - MODE 1)
- Setup both teams in each flag area. Prime your flag by putting your team's drone next to it.
- Try to capture the other team's flag and defend yours. Game ends when everyone runs out of their battery (TEAM WIN BOTH FLAGS or DRAW 1 FLAG EACH).
- Press reset button in each arduino to restart game

### GAME IDEA 2 (multiple flags - MODE 1)
- Team death math. Both teams start from a staging area and try to capture and hold as many flags as you can.
- Game ends when everyone runs out of their battery (TEAM WIN FOR MOST FLAGS).
- Press reset button in each arduino to restart game

### GAME IDEA 3 (DEATH MATCH - MODE 2)
- Up to 8 players (R1-R8) start from a staging area and try to capture as many flags as they can before everyone runs out of battery.
- The one with the most flags captured wins.

### GAME IDEA 4 (SCAVENGER HUNT - MODE 3)
- Up to 8 players (R1-R8) start from a staging area and try to find as many HIDDEN flags as they can before everyone runs out of battery.
- Once a flag is captured it cannot be recaptured by another drone. The one with the most flags captured wins.


# CONTACTS

- https://www.youtube.com/SeekNDFPV
- https://www.facebook.com/SeekNDFPV/


# SUPPORT

If you'd like to help support the project please contribute with ideas.

If you would like to contribute in another way, I would be very grateful as well.
- http://paypal.me/SeekND

Thank you for your support!!
