-- set param ?filter 'QUERIES_IN_CLIENT=30';  -- 30 seconds
set param ?lb     ' blockedInClient: ';
set param ?lq     ' Qid: ';
set param ?lst    ' State: ';
set param ?lsq    ' sqlSrc: ';


select current_timestamp "CURRENT_TIMESTAMP"   -- (1) Now
       ,cast(substr(variable_info,             -- (2) Time in SQL
             position(?lb in variable_info) + char_length(?lb),
             position(' ' in substr(variable_info, 
                          position(?lb in variable_info) + char_length(?lb))))
             as NUMERIC(18)) TIME_IN_SECONDS
       ,cast(substr(variable_info,             -- (3) QID
             position(?lq in variable_info) + char_length(?lq),
             position(' ' in substr(variable_info, 
                          position(?lq in variable_info) + char_length(?lq)))) 
             as varchar(175)CHARACTER SET UTF8) QUERY_ID
       , cast(substr(variable_info,            -- (4) State
             position(?lst in variable_info) + char_length(?lst),
             position(' ' in substr(variable_info, 
                          position(?lst in variable_info) + char_length(?lst))))
             as char(30)) EXECUTE_STATE
       ,cast(substr(variable_info,             -- (5) SQL Source
             position(?lsq in variable_info) + char_length(?lsq),
             char_length(variable_info) - 
                        ( position(?lsq in variable_info) + char_length(?lsq) ))
             as char(256)CHARACTER SET UTF8) SOURCE_TEXT
from table (statistics(NULL, ?filter))
order by 2 descending;
