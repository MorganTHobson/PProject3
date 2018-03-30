// gameoflife.c
// Name: Morgan Hobson
// JHED: mhobson5

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "mpi.h"

#define DEFAULT_ITERATIONS 64
#define GRID_WIDTH  256
#define DIM  16     // assume a square grid

int get_adjacent ( int* grid, int pos ) {

  int adjacent = 0;

  // Convert 1d positions to grid coordinates
  int x = pos % DIM;
  int y = pos / DIM;

  int x_left = (x + DIM - 1) % DIM;
  int x_right = (x + 1) % DIM;
  int y_up = (y + DIM - 1) % DIM;
  int y_down = (y + 1) % DIM;

  // Convert y values to 1d array scale
  y *= 16;
  y_up *= 16;
  y_down *= 16;

  adjacent += grid[ x_left  + y_up ];
  adjacent += grid[ x       + y_up ];
  adjacent += grid[ x_right + y_up ];
  adjacent += grid[ x_left  + y ];
  adjacent += grid[ x_right + y ];
  adjacent += grid[ x_left  + y_down ];
  adjacent += grid[ x       + y_down ];
  adjacent += grid[ x_right + y_down ];
  return adjacent;
}

int main ( int argc, char** argv ) {

  int global_grid[ 256 ] = 
   {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

  // MPI Standard variable
  int num_procs;
  int ID, j;
  int iters = 0;
  int num_iterations;

  // Setup number of iterations
  if (argc == 1) {
    num_iterations = DEFAULT_ITERATIONS;
  }
  else if (argc == 2) {
    num_iterations = atoi(argv[1]);
  }
  else {
    printf("Usage: ./gameoflife <num_iterations>\n");
    exit(1);
  }

  // Messaging variables
  MPI_Status stat;
  // TODO add other variables as necessary
  int* send_up;
  int* send_down;
  int* recv_up;
  int* recv_down;
  int  ID_down;
  int  ID_up;

  // variables for local grid buffer
  int* buffer;
  int buff_size;
  int grid_offset;
  int adjacent;

  // MPI Setup
  if ( MPI_Init( &argc, &argv ) != MPI_SUCCESS )
  {
    printf ( "MPI_Init error\n" );
  }

  MPI_Comm_size ( MPI_COMM_WORLD, &num_procs ); // Set the num_procs
  MPI_Comm_rank ( MPI_COMM_WORLD, &ID );

  assert ( DIM % num_procs == 0 );

  // TODO Setup your environment as necessary
  buff_size = GRID_WIDTH / num_procs;
  buffer = (int*)malloc(buff_size * sizeof(int));
  send_up = (int*)malloc(DIM * sizeof(int));
  send_down = (int*)malloc(DIM * sizeof(int));
  recv_up = (int*)malloc(DIM * sizeof(int));
  recv_down = (int*)malloc(DIM * sizeof(int));

  grid_offset = ID * buff_size;
  ID_down = (ID + num_procs - 1) % num_procs;
  ID_up = (ID + 1) % num_procs;

  for ( iters = 0; iters < num_iterations; iters++ ) {
    // TODO: Add Code here or a function call to you MPI code

    // Gather "seen" rows for adjacent procs
    for ( int i = 0; i < DIM; i++ ) {
      send_down[ i ] = global_grid[ grid_offset + i ];
      send_up[ i ] = global_grid[ (grid_offset + buff_size - DIM + i) % GRID_WIDTH ];
    }


    // Split messages to avoid deadlock
    if ( ID % 2 == 0 ) {
      // order: send then recieve
      MPI_Send( send_down, DIM, MPI_INT, ID_down, 0, MPI_COMM_WORLD );
      MPI_Send( send_up, DIM, MPI_INT, ID_up, 1, MPI_COMM_WORLD );
      MPI_Recv( recv_down, DIM, MPI_INT, ID_down, 1, MPI_COMM_WORLD, &stat );
      MPI_Recv( recv_up, DIM, MPI_INT, ID_up, 0, MPI_COMM_WORLD, &stat );
    } else {
      // order: recieve then send
      MPI_Recv( recv_up, DIM, MPI_INT, ID_up, 0, MPI_COMM_WORLD, &stat );
      MPI_Recv( recv_down, DIM, MPI_INT, ID_down, 1, MPI_COMM_WORLD, &stat );
      MPI_Send( send_up, DIM, MPI_INT, ID_up, 1, MPI_COMM_WORLD );
      MPI_Send( send_down, DIM, MPI_INT, ID_down, 0, MPI_COMM_WORLD );
    }

    // Apply "seen" rows from adjacent procs
    for ( int i = 0; i < DIM; i++ ) {
      global_grid[ (grid_offset + GRID_WIDTH - DIM + i) % GRID_WIDTH ] = recv_down[ i ];
      global_grid[ (grid_offset + buff_size + i) % GRID_WIDTH ] = recv_up[ i ];
    }


    for ( int i = 0; i < buff_size; i++ ) {
      adjacent = get_adjacent( global_grid, grid_offset + i );
      switch(adjacent) {
        case 3:
          buffer[i] = 1;
          break;
        case 2:
          buffer[i] = global_grid[grid_offset + i];
          break;
        default:
          buffer[i] = 0;
      }
    }

    // Set grid region for next iteration
    for ( int i = 0; i < buff_size; i++ ) {
      global_grid[grid_offset + i] = buffer[i];
    }

    // Output the updated grid state
    if ( ID == 0 ) {
      for ( int id = 1; id < num_procs; id++ ) {
        MPI_Recv( buffer, buff_size, MPI_INT, id, 2, MPI_COMM_WORLD, &stat );
        for ( int j = 0; j < buff_size; j++ ) {
          global_grid[ id * buff_size + j ] = buffer[ j ];
        }
      }

      printf ( "\nIteration %d: final grid:\n", iters );
      for ( j = 0; j < GRID_WIDTH; j++ ) {
        if ( j % DIM == 0 ) {
          printf( "\n" );
        }
        printf ( "%d  ", global_grid[j] );
      }
      printf( "\n" );
    } else {
      MPI_Send( buffer, buff_size, MPI_INT, 0, 2, MPI_COMM_WORLD );
    }
  }

  // TODO: Clean up memory

  free( buffer );
  free( send_up );
  free( send_down );
  free( recv_up );
  free( recv_down );

  MPI_Finalize(); // finalize so I can exit
}
