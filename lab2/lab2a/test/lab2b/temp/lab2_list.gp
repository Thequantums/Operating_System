#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2_list-1.png ... cost per operation vs threads and iterations
#	lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#	lab2_list-4.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","



# number of threads for mutex and spin-lock vs. number of operation per second (throughput)
set title "List-1: Throughput vs. Number of threads"
set xlabel "number of threads"
set xrange [1:20]
set logscale x 2
set ylabel "Throughput (op/ns)"
set logscale y 10
set output 'lab2b_1.png'

#Grep for Mutex synchronized list operations, 1,000 iterations, 1,2,4,8,12,16,24 threads
#Grep for Spin-lock synchronized list operations, 1,000 iterations, 1,2,4,8,12,16,24 threads
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'Mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'Spin Lock' with linespoints lc rgb 'green'



# number of threads vs. average time spending waiting to get lock and nu average time per operation

set title "List-4: Threads vs. average time waiting & threads vs. average time/ops (mutex)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "mean time ns"
set logscale y 10
set output 'lab2b_2.png'

# note that unsuccessful runs should have produced no output
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'average time/ops' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'average wait time' with linespoints lc rgb 'red', \
     



set title "List-3: number of threads vs number of iterations (successful outputs)"
set logscale x 2
set xrange [0.75:]
set xlabel "number of threads"
set ylabel "iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'unprotected w/yields=id' with points lc rgb 'red', \
    "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'mutex w/yields=id' with points lc rgb 'green', \
    "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using (3):($3) \
	title 'spin lock w/yields=id' with points lc rgb 'blue', \



set title "List-4: number of threads vs throughput (mutex) (iterations=1000)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_4.png'

plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '1 list' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '4 list' with linespoints lc rgb 'red' \
     "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '8 list' with linespoints lc rgb 'green' \	
     "< grep  'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '16 list' with linespoints lc rgb 'orange' \



set title "List-5: number of threads vs throughput (spin lock) (iterations=1000)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_5.png'

plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '1 list' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '4 list' with linespoints lc rgb 'red' \
     "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '8 list' with linespoints lc rgb 'green' \	
     "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '16 list' with linespoints lc rgb 'orange' \




