#include "fact.h"


#include "fts_task.h"


std::string Fact::get_name() const {
    return task->get_fact_name(fact);
}



bool Fact::is_mutex(const Fact &other) const{
    return task->are_facts_mutex(fact, other.fact);
}
