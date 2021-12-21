Eric Wang, V00913734.
-------------------------------------------------------------------
To run my program, please 'make' on the folder in which
the 4 c files and makefile saves.
-------------------------------------------------------------------
To access the image info:
./diskinfo <your image filename>
-------------------------------------------------------------------
To access the image list:
./disklist <your image filename>
-------------------------------------------------------------------
To get a file from the image:
./diskget <your image file> <filename on the root folder>

Note: We are not required to implement copying a file from the 
subdirectory of the image file to the current Linux folder, so
please do not try that. Instead, copy a file from the 
root folder of the image file only.
-------------------------------------------------------------------
To put a file in your current Linux folder to the root of the image:
./diskput <your image file> <filename>

Note: Please do not attach '/' on the file name as a prefix. Type
the file name only as required in the assignment instruction PDF.
-------------------------------------------------------------------
To put a file in your current Linux folder to the image along a path:
./diskput <your image file> <path/filename>
-------------------------------------------------------------------
Thank you!
