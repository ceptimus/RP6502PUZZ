# RP6502PUZZ
Sliding block puzzle games for the picocomputer RP6502

You've likely played with a physical puzzle - a common one is the '15 puzzle' where you slide around little plastic tiles to try and arrange numbers, or a picture, into order.  The classic 15 puzzle has fifteen square tiles arranged in a 4 x 4 grid, with one empty space which allows the tiles to be slid around.
On the picocomputer, you slide a piece (or several pieces at once) by pointing at a piece with the mouse, and clicking the left mouse button.
The game isn't limited to square pieces - there can be 2x1 pieces (like a domino), L-shaped pieces and so on.
There is a menu which displays a choice of fourteen puzzles (more will be added) of varying difficulty.  Start with the '15 puzzle' if you're a beginner.  Some of the puzzles just have coloured blocks, but others have images - and solving those involves arranging the image in the correct order - a bit like a jigsaw puzzle.
There is the option to save part-completed puzzles, if you want to come back to them later.
It's also easy to create your own new puzzles, either using a paint program and a text editor, or by using an automatic tool which scrambles any suitable image into a new puzzle automatically.

# Installation
You can put the files in the root directory (folder) of your USB memory stick if you want, but it's neater to create a folder named, for example, PUZZ and put all the files in there.
In your chosen folder put the executable puzz.rp6502 and all the puzzle files ##.puzz where ## is a two-digit number in the range 00 to 43.
To run the game, cd to your folder, and enter the command: load puzz.rp6502
