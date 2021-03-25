CS 3013 Project 3

Jason Dykstra, Colby Frechette

For project 3, we had to implement our own versions of malloc, free, init, and destroy with names walloc, free, init, and destroy. We had to past several test cases defined by the project starting materials. We also had to use a doubly linked list to make a list of headers of chunks that were able to be allocated and freed. In order to do all this, our doubly linked list connected headers that established if they were headers of free or allocated chunks, and iterated over these to free and allocate spaces.

As far as we know, the project has no defects, and passes all test cases. The output matches the output provided in the starting materials and can be found in output.txt.