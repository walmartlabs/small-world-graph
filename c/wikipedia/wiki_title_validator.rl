#include "wiki_title_validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

%%{ 
  machine title_matcher;
  write data;

  action invalid_title {
    useful_title = false;
    fbreak;
  }
}%%

bool useful_title(const char* p, const char* pe)
{
  int cs;
  bool useful_title = true;
  const char* eof = pe;
  %%{
    number = digit+;
    number_page = digit+ space+ '(number)';
    year = digit{4} (space+ 'BC')? ;
    month = ('january'i | 'february'i | 'march'i | 'april'i | 'may'i | 'june'i | 'july'i | 'august'i | 'september'i | 'october'i | 'november'i | 'december'i); 
    abbreviated_month = ('jan'i | 'feb'i | 'mar'i | 'apr'i | 'jun'i | 'jul'i | 'aug'i | 'sep'i | 'oct'i | 'nov'i | 'dec'i );
    date = (month | abbreviated_month)
           space+ 
           digit{1,2};
    invalid_title = ((print* ':' print*) | date | year | number | number_page | month ) %invalid_title;
    main := invalid_title;

    write init;
    write exec;
  }%%

  return useful_title;
}
