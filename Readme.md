**Title**: Instant Tuning for a Manual Tuner

**Authors**: Connie Stillinger W6EFI and Robert Melville WB3EFT

**Abstract**:

A random wire with a small manual tuner can be an effective antenna setup for portable operations.    Add a portable network analyzer or small VNA to your kit and tuning can be made more precise, but is still a matter of trial and error, albeit guided.   In this presentation we describe a method for going straight to the optimum tuner settings, giving a previously-gathered one-time characterization of the tuner and a one-time sweep of the antenna.  This method is an adaptation of Melville and Hamilton, "Silent Tuning: Matching a transmitter to an antenna
without emitting a signal"  (MILCOM 2021)

**Other Notes**

Have included the MFJ106010 matchbox "personality" file.  These values measured the same across a couple of instances of the MFJ 16010 tuner.   Note that this version includes blocks for both S12 and S21 which are redundant.    Will fix this in the future.

The code IS NOT OPTIMIZED.   Port to python is underway.

So far this has been presented to the Palo Alto Amateur Radio Association (PAARA), the San Mateo Radio Club (SMRC), and the North American DX Club (NADXCC).   I am very grateful for the interest and input I received, which has been incorporated into the current updated version.

To compile the program: gcc -o match match.c -lm 

To run the program: ./match tuner_personality_file antenna_file

73 es GL de W6EFI



&nbsp;
&nbsp;

_In memoriam WA2DKJ and K2JAO ... whatever band you guys are listening on now we hope you are proud_
