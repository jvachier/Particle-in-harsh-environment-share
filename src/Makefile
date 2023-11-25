CC = g++-13 -O3 -std=c++17
CFLAGS = -Wall -g -fopenmp -fopenmp-simd

main: main.o oldtonew.o concentration_field_density.o concentration_field.o initialization.o initialization_fcts.o memory_allocation.o delocate_memory.o print_position.o print_initial_position.o
	$(CC) $(CFLAGS) -o main.out main.o oldtonew.o concentration_field_density.o concentration_field.o initialization.o initialization_fcts.o memory_allocation.o delocate_memory.o print_position.o print_initial_position.o

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

oldtonew.o: oldtonew.cpp
	$(CC) -c oldtonew.cpp

concentration_field_density.o: concentration_field_density.cpp
	$(CC) $(CFLAGS) -c concentration_field_density.cpp

concentration_field.o: concentration_field.cpp
	$(CC) $(CFLAGS) -c concentration_field.cpp

initialization.o: initialization.cpp
	$(CC) $(CFLAGS) -c initialization.cpp

initialization_fcts.o: initialization_fcts.cpp
	$(CC) $(CFLAGS) -c initialization_fcts.cpp

memory_allocation.o: memory_allocation.cpp
	$(CC) $(CFLAGS) -c memory_allocation.cpp

delocate_memory.o: delocate_memory.cpp
	$(CC) $(CFLAGS) -c delocate_memory.cpp

print_position.o: print_position.cpp
	$(CC) $(CFLAGS) -c print_position.cpp

print_initial_position.o: print_initial_position.cpp
	$(CC) $(CFLAGS) -c print_initial_position.cpp


clean:
	rm *.o