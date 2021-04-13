#!/usr/bin/env python3

import os
import string
import sys
import time
from typing import Tuple

import matplotlib.pyplot as plt

test_battery = ["01-main", "02-switch", "11-join", "12-join-main", "21-create-many",
				"22-create-many-recursive", "23-create-many-once", "31-switch-many",
				"32-switch-many-join", "33-switch-many-cascade.c", "51-fibonacci"]
args = sys.argv

# Number of iterations per test, with the same parameters, of which the average is taken
iterations_number = 10

# Select the ID of the test to execute
test_number = int(args[1])
test_name = test_battery[test_number]

x_val = int(args[2])

dir_thread_path = args[3] + "/"
dir_pthread_path = dir_thread_path


def run_test(test_id: int, test_args: string) -> Tuple[float, float]:
	thread_time = 0
	pthread_time = 0
	for j in range(iterations_number):
		t1 = time.clock_gettime(time.CLOCK_MONOTONIC_RAW)
		os.system(dir_thread_path + test_name + test_args)
		t2 = time.clock_gettime(time.CLOCK_MONOTONIC_RAW)
		thread_time = t2 - t1

		t1 = time.clock_gettime(time.CLOCK_MONOTONIC_RAW)
		os.system(dir_pthread_path + test_name + "-pthread" + test_args)
		t2 = time.clock_gettime(time.CLOCK_MONOTONIC_RAW)
		pthread_time = t2 - t1
	pthread_time /= iterations_number
	thread_time /= iterations_number
	return pthread_time, thread_time


def graph_thread():
	thread_times = []
	pthread_times = []
	for i in range(x_val):
		test_args = " " + str(i) + " 10"
		thread, pthread = run_test(test_number, test_args)
		thread_times.append(thread)
		pthread_times.append(pthread)

	# draw graph
	plt.plot(thread_times, color='blue', label='thread')
	plt.plot(pthread_times, color='red', label='pthread')
	plt.xlabel("First CLI argument (number of threads/iterations)")
	plt.ylabel("Time (s)")
	plt.title("Performance of " + test_name + ", average of " +
			  str(iterations_number) + " iterations")
	plt.legend()
	plt.show()


graph_thread()
