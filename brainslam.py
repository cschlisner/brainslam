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


# print(program)
# lets grab the program intermediate rep 
hax = open("hax.bfbs", "w")
for p in program:
	hax.write(p)
hax.close()


def buildbracemap(code):
	temp_bracestack, bracemap = [], {}
	for position, command in enumerate(code):
		if command == "5": 
			temp_bracestack.append(position)
			# print(temp_bracestack, bracemap)
		if command == "6":
			start = temp_bracestack.pop()
			bracemap[start] = position
			bracemap[position] = start
			# print(temp_bracestack, bracemap)

	return bracemap

print(program)
bracemap = buildbracemap(program)
cells, codeptr, cellptr = [0], 0, 0

while codeptr < len(program):
	command = program[codeptr]
	# print(command,end=',')

	if command == "1":
		cellptr += 1
		if cellptr == len(cells): cells.append(0)

	if command == "2":
		cellptr = 0 if cellptr <= 0 else cellptr - 1

	if command == "3":
		cells[cellptr] = cells[cellptr] + 1 if cells[cellptr] < 255 else 0
		print(cells)

	if command == "4":
		cells[cellptr] = cells[cellptr] - 1 if cells[cellptr] > 0 else 255
		print(cells)


	if command == "5" and cells[cellptr] == 0: codeptr = bracemap[codeptr]
	if command == "6" and cells[cellptr] != 0: codeptr = bracemap[codeptr]
	if command == "7": sys.stdout.write(chr(cells[cellptr]))
	if command == "8": 
		try:
			# Read a character from the input stream.
			char = ord(inputStream.read(1)) % 256
			if char == 26:
				# Detect CTRL+Z, and make that EOF as well.
				eofReached = True
				pass
			else:
				# Set the current byte to the inputted character.
				cells[cellptr] = char
		except:
			pass
		
	codeptr += 1

