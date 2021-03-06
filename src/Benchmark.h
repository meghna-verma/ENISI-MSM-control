/* Code referenced from:
 *  http://coliru.stacked-crooked.com/a/4897b5a72d243df2
 * and discussed via:
 *  https://ngathanasiou.wordpress.com/2015/04/01/benchmarking-in-c/
 * Reused with minor modifications
 */
#ifndef ENISI_MSM_BENCHMARK_H
#define ENISI_MSM_BENCHMARK_H

#include <map>
#include <list>
#include <memory>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <cstddef>
#include <utility>
#include <fstream>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <type_traits>

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

namespace bmk
{
  using std::map; 
  using std::pair;
  using std::vector; 
  using std::string;
  using std::size_t; 
  using std::ostream; 
  using std::is_same; 
  using std::forward; 
  using std::ofstream; 
  using std::enable_if; 
  using std::unique_ptr; 
  using std::result_of; 
  using std::initializer_list; 
  using std::remove_reference; 
  using std::integral_constant;
  using std::chrono::time_point; 
  using std::chrono::duration_cast; 

  /// folly function to avoid optimizing code away 
#ifdef _MSC_VER
  #pragma optimize("", off)
  template <class T>
  void doNotOptimizeAway(T&& datum) 
  {
    datum = datum;
  }
  #pragma optimize("", on)
#else
  template <class T>
  void doNotOptimizeAway(T&& datum) 
  {
    asm volatile("" : "+r" (datum));
  }
#endif

  /// calculate the mean value
  template<typename C>
  long unsigned int mean(C&& data) 
  {
    auto sum = std::accumulate(
      begin(data), 
      end(data),
      (typename remove_reference< decltype( *begin(data) ) >::type)0
    ); 
    auto meanval = sum / data.size();
    return meanval.count();
  }

  /// get the name of the chrono time type
  template<typename T> string time_type()                            { return "unknown";      }
  template<          > string time_type<std::chrono::nanoseconds >() { return "nanoseconds";  }
  template<          > string time_type<std::chrono::microseconds>() { return "microseconds"; }
  template<          > string time_type<std::chrono::milliseconds>() { return "milliseconds"; }
  template<          > string time_type<std::chrono::seconds     >() { return "seconds";      }
  template<          > string time_type<std::chrono::minutes     >() { return "minutes";      }
  template<          > string time_type<std::chrono::hours       >() { return "hours";        }

  template< class TimeT=std::chrono::milliseconds, class ClockT=std::chrono::steady_clock >
  class timeout
  {
  private:
    TimeT _total;
    decltype(ClockT::now()) _start;
  public:
    timeout(): _total(0) {}
    void tic()
    {
      _start = ClockT::now();
    }
    void toc()
    {
      _total += duration_cast<TimeT>(ClockT::now() - _start);
    }
    TimeT duration() const
    {
      return _total;
    }
  };

  //template<class TimeT = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock>
    //using timeout_ptr = unique_ptr < timeout < TimeT, ClockT >	> ;

  template <class TimeT=std::chrono::milliseconds, class ClockT=std::chrono::steady_clock>
  struct timeout_ptr
  {
    typedef unique_ptr < timeout < TimeT, ClockT > > type;
  };


  namespace detail
  {
    /**
    * @ class experiment_impl
    * @ brief implementation details of an experiment
    */
    template<class TimeT, class FactorT>
    struct experiment_impl
    {
      string                      _fctName; 
      map<FactorT, vector<TimeT> > _timings;
    
      experiment_impl(string const& factorName)
	: _fctName(factorName)
      { }

      // implementation of forwarded functions --------------------
      void print(ostream& os) const
      {
	string token{ "" }; 

	os << ", 'factor_name' : '" << _fctName << "'"; 
	// print the factor list
	os << ", 'factors' : [ "; 
	for (auto&& Pair : _timings)
	{
	  os << token; 
	  os << Pair.first; 
	  token = ", "; 
	}
	os << " ]"; 
	// print the timings
	token.clear(); 
	os << ", 'timings' : [ ";
	for (auto&& Pair : _timings)
	{
	  os << token; 
	  os << mean(Pair.second);
	  token = ", ";
	}
	os << " ]";
      }

    protected:
      ~experiment_impl() = default; 
    };

    template<class TimeT>
    struct experiment_impl < TimeT, void >
    {
      vector<TimeT> _timings;

      experiment_impl(size_t nSample)
	: _timings(nSample)
      { }
      
      // implementation of forwarded functions --------------------
      void print(ostream& os) const
      {
	string token{ "" };
	// print the timings
	os << ", 'timings' : [ ";
	for (auto&& elem : _timings)
	{
	  os << token;
	  os << elem.count();
	  token = ", ";
	}
	os << " ]";
      }

    protected:
      ~experiment_impl() = default;
    };

    /**
    * @ class experiment
    * @ brief base class to serve as an experiment concept
    */
    struct experiment
    {
      virtual ~experiment()
      { }

      // forwarded functions --------------------------------------
      virtual void print(ostream& os) const = 0;
    };

    template<class TimeT, class ClockT>
    struct measure
    {
      template<class F>
      inline static auto duration(F callable)
	-> typename enable_if < !is_same <
	decltype(callable()), timeout_ptr<TimeT, ClockT>>::value, TimeT > ::type
      {
	auto start = ClockT::now();
	callable();
	return duration_cast<TimeT>(ClockT::now() - start);
      }

      template<class F>
      inline static auto duration(F callable)
	-> typename enable_if < is_same < 
	decltype(callable()), timeout_ptr<TimeT, ClockT> >::value, TimeT > ::type
      {
	auto start = ClockT::now();
	auto tOut  = callable();
	return (duration_cast<TimeT>(ClockT::now() - start)) - tOut->duration();
      }

      template<class F, typename FactorT>
      inline static auto duration(F callable, FactorT&& factor)
	-> typename enable_if < !is_same < 
	decltype(callable(forward<FactorT>(factor))), timeout_ptr<TimeT, ClockT>
	>::value, TimeT >::type
      {
	auto start = ClockT::now();
	callable(forward<FactorT>(factor));
	return duration_cast<TimeT>(ClockT::now() - start);
      }

      template<class F, typename FactorT>
      inline static auto duration(F callable, FactorT&& factor)
	-> typename enable_if < is_same < 
	decltype(callable(forward<FactorT>(factor))), timeout_ptr<TimeT, ClockT>
	>::value, TimeT >::type
      {
	auto start = ClockT::now();
	auto tOut  = callable(forward<FactorT>(factor));
	return (duration_cast<TimeT>(ClockT::now() - start)) - tOut->duration();
      }
    };

    /**
    * @ class experiment_model
    * @ brief abrastraction for a single sampling process
    */
    template <
      class TimeT, class ClockT, class FactorT = void
    >
    //struct experiment_model final
    struct experiment_model 
      : experiment
      , experiment_impl < TimeT, FactorT >
    {
      // construction - destruction -------------------------------
      template<class F>
      experiment_model(size_t nSample, F callable)
	: experiment_impl<TimeT, void>(nSample)
      {
	for (size_t i = 0; i < nSample; i++)
	{
	  experiment_impl<TimeT, FactorT>::_timings[i] = measure<TimeT, ClockT>
	    ::duration(callable);
	}
      }

      template<class F>
      experiment_model(
	size_t nSample, F callable, 
	string const& factorName, initializer_list<FactorT>&& factors)
	: experiment_impl<TimeT, FactorT>(factorName)
      {
	for (auto&& factor : factors)
	{
	  experiment_impl<TimeT, FactorT>::_timings[factor].reserve(nSample);
	  for (size_t i = 0; i < nSample; i++)
	  {
	    experiment_impl<TimeT, FactorT>::_timings[factor].push_back(
	      measure<TimeT, ClockT>::duration(callable, factor));
	  }
	}
      }

      template<class F, class It>
      experiment_model(size_t nSample, F callable, string const& factorName, It beg, It fin)
	: experiment_impl<
	  TimeT, typename remove_reference<decltype(*beg)>::type>(factorName)
      {
	while (beg != fin)
	{
	  experiment_impl<TimeT, FactorT>::_timings[*beg].reserve(nSample);
	  for (size_t i = 0; i < nSample; i++)
	  {
	    experiment_impl<TimeT, FactorT>::_timings[*beg].push_back(
	      measure<TimeT, ClockT>::duration(callable, *beg));
	  }
	  ++beg;
	}
      }

      // forwarded functions --------------------------------------
      void print(ostream& os) const 
      {
	experiment_impl<TimeT, FactorT>::print(os);
      }

    };
  } // ~ namespace detail


  /**
  * @ class benchmark
  * @ brief organizes the execution and serialization of timing experiments
  */
  template <
    typename TimeT = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock
  >
  class benchmark
  {
    vector<pair<string, unique_ptr<detail::experiment>>> _data; 

  public:
    // construction - destruction -----------------------------------
    benchmark()                 = default; 
    benchmark(benchmark const&) = delete; 

    //// run experiments ----------------------------------------------
    template<class F>
    void run(string const& name, size_t nSample, F callable)
    {
      _data.emplace_back(name, make_unique< 
	detail::experiment_model<TimeT, ClockT>>(nSample, callable));
    }

    template<class FactorT, class F>
    void run(
      string const& name, size_t nSample, F callable, 
      string const& factorName, initializer_list<FactorT>&& factors)
    {
      _data.emplace_back(name, make_unique<detail::experiment_model<TimeT, ClockT, FactorT>>(
	nSample, callable, factorName, forward<initializer_list<FactorT>&&>(factors)));
    }

    template<class F, class It>
    void run(
      string const& name, size_t nSample, 
      F callable, string const& factorName, It beg, It fin)
    {
      _data.emplace_back(name, make_unique<detail::experiment_model<TimeT, ClockT,
	typename remove_reference<decltype(*beg)>::type>>(
	nSample, callable, factorName, beg, fin));
    }

    // utilities ----------------------------------------------------
    void print(const char* benchmarkName, ostream& os) const
    {
      for (auto&& Pair : _data)
      {
	os << "{ 'benchmark_name' : '" << benchmarkName << "'";
	os << ", 'experiment_name' : '" << Pair.first << "'";
	os << ", 'time_type' : '" << time_type<TimeT>() << "'";
	Pair.second->print(os);
	os << " } \n";
      }
    }

    void serialize(
      const char* benchmarkName, const char *filename,
      std::ios_base::openmode mode = ofstream::out) const
    {
      ofstream os;
      os.open(filename, mode);
      for (auto&& Pair : _data)
      {
	os << "{ 'benchmark_name' : '" << benchmarkName << "'";
	os << ", 'experiment_name' : '" << Pair.first << "'";
	os << ", 'time_type' : '" << time_type<TimeT>() << "'";
	Pair.second->print(os);
	os << " } \n";
      }
      os.close(); 
    }
  };

} // ~ namespace bmk


template<class Cont>
std::unique_ptr< bmk::timeout<std::chrono::nanoseconds> > 
CommonUse(int nElems) 
{
    auto to = make_unique < bmk::timeout<std::chrono::nanoseconds> >();
    // ------------------------------------------
    to->tic(); 
	Cont cont;
	std::random_device rd;
	std::mt19937 eng{ rd() };
	std::uniform_int_distribution<int> distr(0, nElems);
    to->toc(); 
    //-------------------------------------------
 
    for (int i = 0, val; i < nElems; i++) {
	val = distr(eng);
	cont.insert(std::find(
	    std::begin(cont), std::end(cont), val), val);
    }
    
    while (nElems) {
	auto it = std::begin(cont);
	std::advance(it, distr(eng) % nElems);
	cont.erase(it);
	--nElems;
    }
    return to; 
}

#endif
