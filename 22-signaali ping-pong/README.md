# Signal ping-pong
Creates copies of itself based on the amount given in arguments. Copies and the main program communicate between each other with signals.
The copies send SIGUSR1-signals to main program, which sends SIGUSR2-signals to all of the copies. The copies quit after receiving SIGUSR2. Main program waits all the copies to end and then quits itself.