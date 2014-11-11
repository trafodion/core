-- set param ?filter 'INACTIVE_QUERIES=30';  -- 30 seconds
set param ?ll   ' lastActivity: ';
set param ?lq   ' Qid: ';
set param ?lsq  ' sqlSrc: ';
select current_timestamp "CURRENT_TIMESTAMP"   -- (1) Now
      ,cast(substr(variable_info,               -- (2) Last Activity
             position(?ll in variable_info) + char_length(?ll),
             position(' ' in substr(variable_info, 
                          position(?ll in variable_info) + char_length(?ll))))
            as NUMERIC(18) ) LAST_ACTIVITY_SECS
      ,cast(substr(variable_info,               -- (3) QID
             position(?lq in variable_info) + char_length(?lq),
             position(' ' in substr(variable_info, 
                          position(?lq in variable_info) + char_length(?lq)))) 
            as varchar(175) CHARACTER SET UTF8) QUERY_ID
      ,cast(substr(variable_info,             -- (5) SQL Source
             position(?lsq in variable_info) + char_length(?lsq),
             char_length(variable_info) - 
                        ( position(?lsq in variable_info) + char_length(?lsq) ))
            as char(256) CHARACTER SET UTF8) SOURCE_TEXT          
from table (statistics(NULL, ?filter)) 
order by 2 descending;
