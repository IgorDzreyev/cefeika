// -*- C++ -*-
// Copyright (C) Dmitry Igrishin
// For conditions of distribution and use, see files LICENSE.txt or pgfe.hpp

#include "unit.hpp"

#include <dmitigr/pgfe/sql_string.hpp>

int main(int argc, char* argv[])
{
  namespace pgfe = dmitigr::pgfe;
  using namespace dmitigr::test;

  try {
    const unsigned long iteration_count = (argc >= 2) ? std::stoul(argv[1]) : 1;
    std::unique_ptr<pgfe::Sql_string> s;
    using Counter = std::remove_const_t<decltype (iteration_count)>;
    for (Counter i = 0; i < iteration_count; ++i) {
      s = pgfe::Sql_string::make("SELECT :list_"
                                 " FROM :t1_ t1 JOIN :t2_ t2 ON (t1.t2 = t2.id)"
                                 " WHERE :where_");

      s->replace_parameter("list_", "t1.id id, t1.age age, t2.dat dat");
      s->replace_parameter("t1_", "table1");
      s->replace_parameter("t2_", "table2");
      s->replace_parameter("where_", "t1.nm = :nm AND t2.age = :age");
    }
    const auto modified_string = s->to_string();
  } catch (const std::exception& e) {
    report_failure(argv[0], e);
    return 1;
  } catch (...) {
    report_failure(argv[0]);
    return 1;
  }
}
