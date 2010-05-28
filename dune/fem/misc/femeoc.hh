#ifndef DUNE_FEMEOC_HH
#define DUNE_FEMEOC_HH

#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include <dune/common/fvector.hh>
#include <dune/fem/io/file/iointerface.hh>

namespace Dune
{

/**  
    @ingroup HelperClasses
    \brief Write a self contained tex table 
    for eoc runs with timing information.
    
    Constructor takes base file name for the tex file and
    generates two files:
    filename_timestamp_main.tex and filename_timestamp_body.tex or
    without the time stamp (by default) in the name depending on 
    the parameter fem.io.eocFileTimeStamp.
    A time stamp is added to the base file name to prevent the
    overwriting of a valuable eoc data from the previous simulation.
    The file filename_timestamp_body.tex holds the actual body
    of the eoc table which is included in filename_timestamp_main.tex
    but can also be used to combine e.g. runs with different
    parameters or for plotting using gnuplot.

    The class is singleton and thus new errors for eoc
    computations can be added in any part of the program.
    To add a new entry for eoc computations use one of the
    addEntry methods. These return a unique unsigned int
    which can be used to add error values to the table
    with the setErrors methods.
    The method write is used to write a single line
    to the eoc table.
 */
class FemEoc
{
  std::ofstream outputFile_;
  int level_;
  std::vector<double> prevError_;
  std::vector<double> error_;
  std::vector<std::string> description_;
  double prevh_;
  bool initial_;
  std::vector<int> pos_;
  FemEoc() :
    outputFile_()
  , level_(0)
  , prevError_(0)
  , error_(0)
  , description_(0)
  , prevh_(0)
  , initial_(true)
  , pos_(0)
  {
  }
  ~FemEoc() {
    outputFile_.close();
  }

  void init(const std::string& path,
            const std::string& name, const std::string& descript) 
  {
    if (MPIManager::rank() != 0) return;
    IOInterface::createPath(path);
    init(path+"/"+name,descript);
  }

  void init(const std::string& filename, const std::string& descript)
  {
    if (MPIManager::rank() != 0) return;

    std::string name = filename; 

    // if needed add the time stamp to the file name 
    // to prevent eoc results to be overwritten
    if ( Parameter::getValue<int>("fem.io.eocFileTimeStamp", 0) )
    {
      time_t seconds = time(0);
      struct tm *ptm = localtime( &seconds );
      char timeString[20];
      strftime( timeString, 20, "_%d%m%Y_%H%M%S", ptm );
      name = filename + std::string(timeString); 
    }

    if (!outputFile_.is_open()) 
    {
      std::ofstream main((name+"_main.tex").c_str());
      if (!main) {
        std::cerr << "Could not open file : "
                  << (name+"_main.tex").c_str() 
                  << " ... ABORTING" << std::endl;
        abort();
      }

      std::string filestreamBody;
      filestreamBody = name + "_body.tex";
      outputFile_.open(filestreamBody.c_str(), std::ios::out);
      main << "\\documentclass[12pt,english]{article}\n"
           << "\\usepackage[T1]{fontenc}\n"
	         << "\\usepackage[latin1]{inputenc}\n"
	         << "\\usepackage{setspace}\n"
	         << "\\onehalfspacing\n"
	         << "\\makeatletter\n"
	         << "\\providecommand{\\boldsymbol}[1]{\\mbox{\\boldmath $#1$}}\n"
	         << "\\providecommand{\\tabularnewline}{\\\\}\n"
	         << "\\usepackage{babel}\n"
	         << "\\makeatother\n"
	         << "\\begin{document}\n"
           << "\\begin{center}\\large\n"
           << "\n\\end{center}\n\n"
           << "\\input{"
           << filestreamBody
           << "}\n";
      main << "\\end{tabular}\\\\\n\n"
	         << "\\end{document}\n" << std::endl;
      main.close();	

      // write together with table the description
      // of the scheme and problem in the simulation
      outputFile_ << descript;
    } 
    else 
    {
      std::cerr << "Could not open file : "
                << " already opened!"
                << " ... ABORTING" << std::endl;
      abort();
    }
  }
  template <class StrVectorType>
  size_t addentry(const StrVectorType& descript,size_t size) 
  {
    if (!initial_) 
      abort();
    pos_.push_back(error_.size());
    for (size_t i=0;i<size;++i) {
      error_.push_back(0);
      prevError_.push_back(0);
      description_.push_back(descript[i]);  
    }
    return pos_.size()-1;
  }
  size_t addentry(const std::string& descript) {
    if (!initial_) 
      abort();
    pos_.push_back(error_.size());
    error_.push_back(0);
    prevError_.push_back(0);
    description_.push_back(descript);  
    return pos_.size()-1;
  }

  template <class VectorType>
  void seterrors(size_t id,const VectorType& err,size_t size) 
  {
    assert(id<pos_.size());
    int pos = pos_[ id ];
    assert(pos+size <= error_.size());

    for (size_t i=0; i<size; ++i)
      error_[pos+i] = err[i];
  }

  template <int SIZE>
  void seterrors(size_t id,const FieldVector<double,SIZE>& err) 
  {
    seterrors(id,err,SIZE);
  }
  
  void seterrors(size_t id,const double& err) {
    int pos = pos_[id];
    error_[pos] = err;
  }

  void writeerr(double h,double size,double time,int counter) {
    if (MPIManager::rank() != 0) return;
    if (initial_) {
	    outputFile_ << "\\begin{tabular}{|c|c|c|c|c|";
      for (unsigned int i=0;i<error_.size();i++) {
        outputFile_ << "|cc|";
      }
      outputFile_ << "}\n"
	        << "\\hline \n"
          << "level & h & size & CPU-time & counter";
      for (unsigned int i=0;i<error_.size();i++) {
        outputFile_ << " & " << description_[i]
                    << " & EOC ";
      }
      outputFile_ << "\n \\tabularnewline\n"
                  << "\\hline\n"
                  << "\\hline\n";
    }
    outputFile_ <<  "\\hline \n"
                << level_ << " & "
                << h      << " & "
                << size   << " & "
                << time   << " & " 
                << counter;
    for (unsigned int i=0;i<error_.size();++i) {
      outputFile_ << " & " << error_[i] << " & ";
      if (initial_) {
        outputFile_ << " --- ";
      }
      else {
        double factor = prevh_/h;
        outputFile_ << log(prevError_[i]/error_[i])/log(factor);
      }
      prevError_[i]=error_[i];
      error_[i] = -1;  // uninitialized
    }
    outputFile_ << "\n"
                << "\\tabularnewline\n"
                << "\\hline \n";
    outputFile_.flush();
    prevh_ = h;
    level_++;
    initial_ = false;
  }

  void writeerr(double h,double size,double time,int counter,
                double avgTimeStep,double minTimeStep,double maxTimeStep) {
    if (MPIManager::rank() != 0) return;
    if (initial_) {
	    outputFile_ << "\\begin{tabular}{|c|c|c|c|c|c|c|c|";
      for (unsigned int i=0;i<error_.size();i++) {
        outputFile_ << "|cc|";
      }
      outputFile_ << "}\n"
	        << "\\hline \n"
          << "level & h & size & CPU-time & counter & avg dt & min dt & max dt";
      for (unsigned int i=0;i<error_.size();i++) {
        outputFile_ << " & " << description_[i]
                    << " & EOC ";
      }
      outputFile_ << "\n \\tabularnewline\n"
                  << "\\hline\n"
                  << "\\hline\n";
    }
    outputFile_ <<  "\\hline \n"
                << level_ << " & "
                << h      << " & "
                << size   << " & "
                << time   << " & " 
                << counter <<" & "
                << avgTimeStep   << " & " 
                << minTimeStep   << " & " 
                << maxTimeStep;
    for (unsigned int i=0;i<error_.size();++i) {
      outputFile_ << " & " << error_[i] << " & ";
      if (initial_) {
        outputFile_ << " --- ";
      }
      else {
        double factor = prevh_/h;
        outputFile_ << log(prevError_[i]/error_[i])/log(factor);
      }
      prevError_[i]=error_[i];
      error_[i] = -1;  // uninitialized
    }
    outputFile_ << "\n"
                << "\\tabularnewline\n"
                << "\\hline \n";
    outputFile_.flush();
    prevh_ = h;
    level_++;
    initial_ = false;
  }

  // do the same calculations as in write, but don't overwrite status 
  void printerr(const double h, 
                const double size, 
                const double time, 
                const int counter,
                std::ostream& out) 
  {
    if (!Parameter::verbose()) return;
	  out << "level:   " << level_  << std::endl;
	  out << "h        " << h << std::endl;
	  out << "size:    " << size << std::endl;
	  out << "time:    " << time << " sec. " << std::endl;
	  out << "counter: " << counter << std::endl;

    for (unsigned int i=0;i<error_.size();++i) 
    {
      out << description_[i] << ":       " << error_[i] << std::endl;
      if (! initial_) 
      {
        const double factor = prevh_/h;
        const double eoc = log(prevError_[i]/error_[i])/log(factor);

        out << "EOC (" <<description_[i] << "): " << eoc << std::endl;
      }
      out << std::endl;
    }
  }
  // do the same calculations as in write, but don't overwrite status 
  void printerr(const double h, 
                const double size, 
                const double time, 
                const int counter,
                const double avgTimeStep,
                const double minTimeStep,
                const double maxTimeStep,
                std::ostream& out) 
  {
    if (!Parameter::verbose()) return;
	  out << "level:   " << level_  << std::endl;
	  out << "h        " << h << std::endl;
	  out << "size:    " << size << std::endl;
	  out << "time:    " << time << " sec. " << std::endl;
	  out << "counter: " << counter << std::endl;
	  out << "avg. time step: " << avgTimeStep << std::endl;
	  out << "min. time step: " << minTimeStep << std::endl;
	  out << "max. time step: " << maxTimeStep << std::endl;

    for (unsigned int i=0;i<error_.size();++i) 
    {
      out << description_[i] << ":       " << error_[i] << std::endl;
      if (! initial_) 
      {
        const double factor = prevh_/h;
        const double eoc = log(prevError_[i]/error_[i])/log(factor);

        out << "EOC (" <<description_[i] << "): " << eoc << std::endl;
      }
      out << std::endl;
    }
  }
 public:
  static FemEoc& instance() {
    static FemEoc instance_;
    return instance_;
  }
  //! open file path/name and write a description string into tex file
  static void initialize(const std::string& path, const std::string& name, const std::string& descript) {
    instance().init(path,name,descript);
  }
  //! open file name and write description string into tex file
  static void initialize(const std::string& name, const std::string& descript) {
    instance().init(name,descript);
  }
  /** \brief add a vector of new eoc values  
   *
   *  \tparam  StrVectorType a vector type with operator[] 
   *           returning a string (a C style array can be used)
   *           the size of the vector is given as parameter
   *  \return  a unique index used to add the error values 
   */
  template <class StrVectorType>
  static size_t addEntry(const StrVectorType& descript,size_t size) {
    return instance().addentry(descript,size);
  }
  /** \brief add a vector of new eoc values  
   *
   *  \tparam  StrVectorType a vector type with size() and operator[]
   *           returning a string
   *  \return  a unique index used to add the error values 
   */
  template <class StrVectorType>
  static size_t addEntry(const StrVectorType& descript) {
    return instance().addentry(descript,descript.size());
  }
  /** \brief add a single new eoc output  
   *
   *  \return  a unique index used to add the error values 
   */
  static size_t addEntry(const std::string& descript) {
    return instance().addentry(descript);
  }
  /** \brief add a single new eoc output  
   *
   *  \return  a unique index used to add the error values 
   */
  static size_t addEntry(const char* descript) {
    return addEntry(std::string(descript));
  }
  /** \brief add a vector of error values for the given id (returned by
   *         addEntry)
   *  \tparam  VectorType a vector type with an operator[] 
   *           returning a double (C style array can be used)
   */
  template <class VectorType>
  static void setErrors(size_t id,const VectorType& err,int size) 
  {
    instance().seterrors(id,err,size);
  }
  /** \brief add a vector of error values for the given id (returned by
   *         addEntry)
   *  \tparam  VectorType a vector type with a size() and an operator[] 
   *           returning a double 
   */
  template <class VectorType>
  static void setErrors(size_t id,const VectorType& err) {
    instance().seterrors(id,err,err.size());
  }
  /** \brief add a vector in a FieldVector of error values for the given id (returned by
   *         addEntry)
   */
  template <int SIZE>
  static void setErrors(size_t id,const FieldVector<double,SIZE>& err) {
    instance().seterrors(id,err);
  }
  /** \brief add a single error value for the given id (returned by
   *         addEntry)
   */
  static void setErrors(size_t id,const double& err) {
    instance().seterrors(id,err);
  }
  /** \brief commit a line to the eoc file 
   *
   *  \param h grid width (e.g. given by GridWith utitlity class)
   *  \param size number of elements in the grid or number of dofs...
   *  \param time computational time
   *  \param counter number of timesteps or iterations for a solver...
   */
  static void write(double h,double size,double time,int counter) 
  {
    instance().writeerr(h,size,time,counter);
  }

  /** \brief commit a line to the eoc file 
   *
   *  \param h grid width (e.g. given by GridWith utitlity class)
   *  \param size number of elements in the grid or number of dofs...
   *  \param time computational time
   *  \param counter number of timesteps or iterations for a solver...
   *  \param avgTimeStep average time step for a ODE solver for one run of the program...
   *  \param minTimeStep minimal time step for a ODE solver for one run of the program...
   *  \param maxTimeStep maximal time step for a ODE solver for one run of the program...
   */
  static void write(double h,double size,double time,int counter,
                    const double avgTimeStep,
                    const double minTimeStep,
                    const double maxTimeStep ) 
  {
    instance().writeerr(h,size,time,counter,avgTimeStep,minTimeStep,maxTimeStep);
  }

  /** \brief commit a line to the eoc file 
   *
   *  \param h grid width (e.g. given by GridWith utitlity class)
   *  \param size number of elements in the grid or number of dofs...
   *  \param time computational time
   *  \param counter number of timesteps or iterations for a solver...
   *  \param out std::ostream to print data to (e.g. std::cout) 
   */
  static void write(const double h,
                    const double size,
                    const double time, 
                    const int counter,
                    std::ostream& out) 
  {
    // print last line to out 
    instance().printerr( h, size, time, counter, out );

    // now write to file 
    instance().writeerr(h,size,time,counter);
  }

  /** \brief commit a line to the eoc file 
   *
   *  \param h grid width (e.g. given by GridWith utitlity class)
   *  \param size number of elements in the grid or number of dofs...
   *  \param time computational time
   *  \param counter number of time steps or iterations for a solver...
   *  \param avgTimeStep average time step for a ODE solver for one run of the program...
   *  \param minTimeStep minimal time step for a ODE solver for one run of the program...
   *  \param maxTimeStep maximal time step for a ODE solver for one run of the program...
   *  \param out std::ostream to print data to (e.g. std::cout) 
   */
  static void write(const double h,
                    const double size,
                    const double time, 
                    const int counter,
                    const double avgTimeStep,
                    const double minTimeStep,
                    const double maxTimeStep,
                    std::ostream& out) 
  {
    // print last line to out 
    instance().printerr( h, size, time, counter, avgTimeStep, minTimeStep, maxTimeStep, out );

    // now write to file 
    instance().writeerr(h,size,time,counter,avgTimeStep,minTimeStep,maxTimeStep);
  }

}; // end class FemEoc

}
#endif
