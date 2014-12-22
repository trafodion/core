/*
 * BufferingAppender.cpp
 *
 * (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <log4cpp/BufferingAppender.hh>
#include <algorithm>
#ifdef LOG4CPP_HAVE_SSTREAM
#    include <sstream>
#else
#    include <strstream>
#endif
#include <memory>

namespace log4cpp
{
   BufferingAppender::BufferingAppender(const std::string name, unsigned long max_size, 
                                        std::unique_ptr<Appender> sink, std::unique_ptr<TriggeringEventEvaluator> evaluator) 
                     :LayoutAppender(name), max_size_(max_size), sink_(std::move(sink)), evaluator_(std::move(evaluator)), lossy_(false)
   {
      max_size_ = (std::max)(1UL, max_size_);
   }
   
   void BufferingAppender::_append(const LoggingEvent& event)
   {
      if (queue_.size() == max_size_)
         if (lossy_)
            queue_.pop_back();
         else
            dump();

      queue_.push_front(event);
      
      if (evaluator_->eval(event))
      {
         dump();
         queue_.clear();
      }
   }

   static const std::string EMPTY;
   
   void BufferingAppender::dump()
   {
      Layout& layout = _getLayout();
      std::ostringstream s;
      //  Solaris 10 CC can't work with const_reverse_iterator
      for(queue_t::reverse_iterator i = queue_.rbegin(), last = queue_.rend(); i != last; ++i)
         s << layout.format(*i);

      LoggingEvent event(EMPTY, s.str(), EMPTY, Priority::NOTSET);
      sink_->doAppend(event);
   }
}

