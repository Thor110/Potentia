## Sieve of Potentia - Statistics

most of these tests are from early versions of the manifest

random image someone sent me a link to at 3653x5480 resolution

1-amorgos.jpg.pagespeed.ce.Mgygi9l3S-.jpg			: 3,844,213 bytes ( 3,846,144 bytes on disk )
1-amorgos.jpg.pagespeed.ce.Mgygi9l3S-.jpg.7z		: 3,843,942 bytes ( 3,846,144 bytes on disk )
1-amorgos.jpg.pagespeed.ce.Mgygi9l3S-.jpg.7z.sieve	: 3,829,453 bytes ( 3,829,760 bytes on disk )
1-amorgos.jpg.pagespeed.ce.Mgygi9l3S-.jpg.sieve		: 3,826,046 bytes ( 3,829,760 bytes on disk )

A file from Jeff Wayne's The War of the Worlds the 1998 PC RTS game, proprietary .WoW archive format.

Early results

MAPS.WoW		: 160,890,897 bytes
MAPS.7z			: 60,791,826 bytes
MAPS.WoW.sieve	: 60,130,147 bytes

The potential utility line - 100mbs or more is when the search time penalty begins to kick in

Latest results for MAPS.WoW since manifest v28

optimised for size

	encoding
7zip - ~18s - 60,791,826 bytes
sieve - 350.1s - 52,996,145 bytes

	decoding
7zip - ~1s
sieve - 202.769s - checked and written in 0.918s

optimised for decoding

	encoding
sieve - 349.5s - 60,130,138 bytes

	decoding
sieve - 4.563s - checked and written in 0.928s

random test - the gutenberg by don quixote

pg996.txt		: 2,391,678 bytes	(2,392,064 bytes on disk)
pg996.7z		: 686,020 bytes		(688,128 bytes on disk)
pg996.txt.sieve	: 683,205 bytes		(684,032 bytes on disk)

pi6.txt stress test, file sourced from : https://www.exeter.ac.uk/v8media/facultysites/ese/physics/pi6.txt

pi6.txt			: 1,111,103 bytes	(1,114,112 bytes on disk)	- original
pi6.7z			: 512,745 bytes		(516,096 bytes on disk)		- 7zipped

this shows the progress from an early v4 to the latest version of the manifest format (the overhead and filter settings)

1 Million Digits of Pi									Theoretical floor distance remaining
														Increase from floor						Decrease from floor
pi6.txt.sieve : 490,473 bytes (491,520 bytes on disk)	: 18.117671424546227%					: -15.338662882564384%

to

pi6.txt.sieve : 421,491 bytes (421,888 bytes on disk)	: 1.5051500213129243%					: -1.482831187380039%

to

pi6.txt.sieve : 419,115 bytes (421,888 bytes on disk)	: 0.932952189210603%					: -0.9243286448826694%

then to

pi6.txt.sieve : 419,096 bytes (421,888 bytes on disk)	: 0.9283765331458117%					: -0.9198369824574799%

and now to

pi6.txt.sieve : 417,139 bytes (417,792 bytes on disk)	: 0.45708395847230887%					: -0.4550042072306833%

another steps reductions

pi6.txt.sieve : 417,061 bytes (417,792 bytes on disk)	: 0.43829968620632354%					: -0.436387003340039%

rope wave updated

pi6.txt.sieve : 417,052 bytes (417,792 bytes on disk)	:

now 1,811 bytes away from the theoretical limit

The theoretical floor for a million digits treated as random is ~415,241 bytes, so this is within 0.43% of it.

A couple of random images

20230830_163945.jpg			: 4,139,682 bytes	(4,141,056 bytes on disk)
20230830_163945.7z			: 4,127,434 bytes	(4,128,768 bytes on disk)
20230830_163945.7z.sieve	: 4,112,541 bytes	(4,116,480 bytes on disk)
20230830_163945.jpg.sieve	: 4,111,592 bytes	(4,112,384 bytes on disk)

20240407_153939.jpg			: 4,587,473 bytes	(4,587,520 bytes on disk)
20240407_153939.7z			: 4,575,201 bytes	(4,575,232 bytes on disk)
20240407_153939.7z.sieve	: 4,559,089 bytes	(4,562,944 bytes on disk)
20240407_153939.jpg.sieve	: 4,558,132 bytes	(4,558,848 bytes on disk)

This test on a python script is 10 bytes larger than 7zip, without the overhead and hash it would be smaller.

control.py					: 1,597 bytes	(4,096 bytes on disk)
control.7z					: 822 bytes		(4,096 bytes on disk)
control.7z.sieve			: 959 bytes		(4,096 bytes on disk)
control.py.sieve			: 832 bytes		(4,096 bytes on disk)

The application itself as a .sieve file

v28 : 6,383,087
v29 : 6,312,557

Which unpacks to 44,542,021 bytes

v30 : 6,311,463
as 7z : 7,774,880 bytes

Which unpacks to 44,582,469 bytes

The payload grew by 40,448 bytes and the sieve file shrunk by 1094 bytes.

v31 : 6,318,477

Which unpacks to 44,621,183 bytes

Payload grew by 38,714 bytes and the sieve file grew by 7,014 bytes.
