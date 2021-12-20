import argparse, random, re, sys

# Function to print a specified error message to STDERR, then exit.
def error(errorMsg):
	sys.stderr.write("\nERROR: "+errorMsg)
	sys.exit()

# Parse command-line options.
parser = argparse.ArgumentParser()
parser.add_argument(
	"program",
	help="file to run as Poetic code"
)
parser.add_argument(
	"-i", "--input",
	metavar="inputFile",
	help="take input from the specified file",
	required=False
)
parser.add_argument(
	"-w", "--wimpmode",
	help="interpret program in wimpmode",
	action="store_true",
	required=False
)
args = parser.parse_args()

# If an input file was specified on the command-line,
# then we use it for input.
# Otherwise, we use STDIN.
if args.input == None:
	inputStream = sys.stdin
else:
	inputStream = open(args.input)

# Open the program file, and convert it to a series of instructions
with open(args.program, encoding="utf8") as f:
	if args.wimpmode:
		program = "".join([c for c in f.read() if c in "0123456789"])
	else:
		program = "".join([c if c.isalpha() else ("" if c=="'" else " ") for c in f.read()])
		program = "".join([str(len(w)) if len(w)!=10 else "0" for w in program.split()])
	program = re.findall(r"((?:[3456]\d)|\d)", program)

# Initializations
instructionPointer = 0
memory = bytearray(30000)
memoryPointer = 0
eofReached = False
hax = open("hax.bfptc", "w")
for p in program:
	hax.write(p)
hax.close()
# Loop through the program.
while True:
	try:
		# Get the next instruction.
		currentInstruction = program[instructionPointer][0]
	except:
		# If we get an error, we've jumped past the end of the program with no END instruction.
		# This is an error.
		error("Unexpected EOF")
	
	try:
		# Try to get the argument, changing it from 0 to 10 if applicable.
		currentArgument = int(program[instructionPointer][1])
		if currentArgument == 0:
			currentArgument = 10
	except:
		# If we get an error, and the argument is required for the command, this is an error.
		if currentInstruction in "3456":
			error("Missing argument")
		else:
			pass
	
	if currentInstruction == "0":
		# 0: END
		# End program execution here. This does not end program execution if it is used as an argument for INC, DEC, FWD, or BAK.
		break
	elif currentInstruction == "1":
		# 1: IF
		# If the current byte is equal to 0, jump execution to after the matching EIF.
		if memory[memoryPointer] == 0:
			# Track the nesting of the loops.
			nested = 1
			while nested:
				instructionPointer += 1
				try:
					if program[instructionPointer][0] == "1":
						nested += 1
					elif program[instructionPointer][0] == "2":
						nested -= 1
				except:
					# If we never find a match for the IF, this is an error.
					error("Mismatched IF/EIF")
		else:
			instructionPointer += 1
	elif currentInstruction == "2":
		# 2: EIF
		# If the current byte is not equal to 0, jump execution to before the matching IF.
		if memory[memoryPointer] != 0:
			# Track the nesting of the loops.
			nested = -1
			while nested:
				instructionPointer -= 1
				try:
					if program[instructionPointer][0] == "1":
						nested += 1
					elif program[instructionPointer][0] == "2":
						nested -= 1
				except:
					# If we never find a match for the EIF, this is an error.
					error("Mismatched IF/EIF")
		else:
			instructionPointer += 1
	elif currentInstruction == "3":
		# 3: INC
		# Increment the value of the current byte by next_digit. If next_digit is 0, the value used instead is 10.
		memory[memoryPointer] = (memory[memoryPointer] + currentArgument) % 256
		instructionPointer += 1
	elif currentInstruction == "4":
		# 4: DEC
		# Decrement the value of the current byte by next_digit. If next_digit is 0, the value used instead is 10.
		memory[memoryPointer] = (memory[memoryPointer] - currentArgument) % 256
		instructionPointer += 1
	elif currentInstruction == "5":
		# 5: FWD
		# Increment the memory pointer by next_digit. If next_digit is 0, the value used instead is 10.
		memoryPointer = (memoryPointer + currentArgument) % len(memory)
		instructionPointer += 1
	elif currentInstruction == "6":
		# 6: BAK
		# Decrement the memory pointer by next_digit. If next_digit is 0, the value used instead is 10.
		memoryPointer = (memoryPointer - currentArgument) % len(memory)
		instructionPointer += 1
	elif currentInstruction == "7":
		# 7: OUT
		# Output the value of the current byte as an ASCII character.
		print( chr(memory[memoryPointer]), end="", flush=True )
		instructionPointer += 1
	elif currentInstruction == "8":
		# 8: IN
		# Read a character from the input stream, and write its ASCII value to the current byte.
		if eofReached:
			# If we have reached EOF,
			# do nothing.
			pass
		else:
			try:
				# Read a character from the input stream.
				char = ord(inputStream.read(1)) % 256
				if char == 26:
					# Detect CTRL+Z, and make that EOF as well.
					eofReached = True
					pass
				else:
					# Set the current byte to the inputted character.
					memory[memoryPointer] = char
			except:
				# If we get an error, there are no more characters to read.
				# This is fine, and this is EOF.
				eofReached = True
				pass
		instructionPointer += 1
	elif currentInstruction == "9":
		# 9: RND
		# Set the current byte to a random value from 0 to 255.
		memory[memoryPointer] = random.randint(0,255)
		instructionPointer += 1