#ifndef ODE_EMPTY_COMMUNICATOR_HPP
#define ODE_EMPTY_COMMUNICATOR_HPP

#include "thread.hpp"
#include <iostream>
#include <vector>
#include <cassert>

class EmptyCommunicator 
{
  EmptyCommunicator() {}
public:
  static EmptyCommunicator & instance () 
  { 
    EmptyCommunicator comm;
    return comm;
  }
  ~EmptyCommunicator() {}
  void set_output(std::ostream &os) {}
  int id() const {return 0;}
  int size() const {return 1;}
  int master() const {return 0;}
  bool rb_empty(int i) {return true;}
  template<class T> void put(int dest, const T& content) {}
  template<class T> void put_all(const T& content) {}
  template<class T> bool get(int source, T& content) {return true;}
  template<class T> void put(int dest, const T* content, int num) {}
  template<class T> void put_all(const T* content, int num) {}
  template<class T> bool get(int source, T* content, int num) {return true;}
  
  void send_request(int dest) {}
  void receive_request(int source) {}
  void send_receive_request(int process) {}
  void send_request_all() {}
  void receive_request_all() {}
  void send_receive_request_all() {}
  void start_communication() {}
  bool finish_communication() {return true;}
  bool start_and_finish_communication() {return true;}
  void barrier() {}
  
  // MPI-wrapper
  void allreduce(int n, double *in, double *out, MPI_Op op) const {
    for (int i=0;i<n;i++)
      out[i]=in[i];
  }
  
  // conversion to MPI EmptyCommunicator
  operator MPI_Comm() const {}
  
  // Timings
  double time() const {return 0.;}
  double communication_time() const {return 0.;}
  double run_time() const {return 0.;}
  const double* idle_times() const {return 0;}
  void reset_timers() {}
  
  // I/O
  void write(const char filename[]) {}
  void read(const char filename[]) {}
};

#endif
