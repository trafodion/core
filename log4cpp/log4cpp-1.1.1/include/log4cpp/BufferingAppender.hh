/*
 * (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#if !defined(h_ebd0ee89_622d_4af1_9a9d_d0e057debe86)
#define h_ebd0ee89_622d_4af1_9a9d_d0e057debe86

#include <log4cpp/LayoutAppender.hh>
#include <log4cpp/TriggeringEventEvaluator.hh>
#include <list>
#include <memory>

namespace log4cpp
{
   class LOG4CPP_EXPORT BufferingAppender : public LayoutAppender 
   {
      public:
         BufferingAppender(const std::string name, unsigned long max_size, std::unique_ptr<Appender> sink,
                           std::unique_ptr<TriggeringEventEvaluator> evaluator);
      
         virtual void close() { sink_->close(); }
         
         bool getLossy() const { return lossy_; }
         void setLossy(bool lossy) { lossy_ = lossy; }

      protected:
         virtual void _append(const LoggingEvent& event);

      private:
         typedef std::list<LoggingEvent> queue_t;
         
         queue_t queue_;
         unsigned long max_size_;
         std::unique_ptr<Appender> sink_;
         std::unique_ptr<TriggeringEventEvaluator> evaluator_;
         bool lossy_;

         void dump();
   };
}

#endif // h_ebd0ee89_622d_4af1_9a9d_d0e057debe86
