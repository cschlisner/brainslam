# Translate brainfuck into poetic using a given dictionary
# Phase 1: use lists of subjects/verbs/etc
# Phase 2: use nltk library to use given corpus and generate valid sentences (with given word length restrictions)

import argparse, random, re, sys
parser = argparse.ArgumentParser(description="Translate brainfuck into computer generated poetry")
parser.add_argument(
	"input",
	metavar="inputFile",
	help="brainfuck source file to translate to poetic",
)
args = parser.parse_args()


src=""

# Read input file 
with open(args.input, encoding="utf8") as sourcef:
    src = sourcef.read()


# lets convert the brainfuck into raw poetic (intermediate numeric representation)
# print(src)

# lol
bf = "><+-.,[]"

src_digits=[]
# [int(d) for d in "3153535353536421535353535364215353535353642153535353536421535353535364215353535353642153535353536421535353535364215353535353642153535353536425375373773753754767374747537570"]

skp=False
cctline = src.strip().split()[0]
for i in range(len(cctline)):
    ins = int(cctline[i])
    nxt = 20
    if (i < len(cctline)-1):
        nxt = int(cctline[i+1])
    comb_ins = ins*10+nxt
    # print(":: %s (%s) %s"%(ins, nxt, comb_ins))
    if skp:
        skp = False 
        continue
    if (ins == 0):
        src_digits.append(10);
    elif (comb_ins < 20):
        skp = True
        src_digits.append(comb_ins)
    else:
        src_digits.append(ins)

print(','.join(str(d) for d in src_digits))

# bag of words -- will be expanded upon
corpus = open("words.txt")
validwords = []
#remove words with apostrophes and dashes
for word in corpus:
    word = word.strip()
    if not word.isalpha():
        continue
    validwords.append(word)
# randomize our words
random.shuffle(validwords)

# make a wordset for each valid Poetic opcode (by index)
wordsets = []

for word in validwords:
    l = len(word)
    if (l >= len(wordsets)):
        for i in range (l-len(wordsets)+1):
            wordsets.append([])
    wordsets[l-1].append(word)

# shuffle our words again
for set in wordsets:
    random.shuffle(set)

# print(validwords)
# print(wordsets)


#loop over a copy of src_digits and make substrings for sentence structures
srcp = src_digits

# writing to this output file 
with open("transpiled.ptc", mode="w") as outputf:
    while len(srcp) > 0:
        slen = random.randint(0,10)
        stnce = srcp[:slen]

        for wrd in stnce:
            outputf.write(random.choice(wordsets[wrd-1])+" ")
        outputf.write("\n")

        srcp = srcp[slen:]


