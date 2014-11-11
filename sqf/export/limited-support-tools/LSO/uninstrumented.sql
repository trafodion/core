-- set param ?filter 'UNMONITORED_QUERIES=30';  -- 30 seconds
set param ?ll   ' lastActivity: ';
set param ?lqt  ' queryType: ';
set param ?lqs  ' subqueryType: ';
set param ?lsts ' StatsType: '; 
set param ?lq   ' Qid: ';
set param ?lsq  ' sqlSrc: ';

select current_timestamp "CURRENT_TIMESTAMP"  -- (1) Now  
      ,cast(substr(variable_info,             -- (2) Last Activity
             position(?ll in variable_info) + char_length(?ll),
             position(' ' in substr(variable_info, 
                          position(?ll in variable_info) + char_length(?ll))))
             as NUMERIC(18) ) LAST_ACTIVITY_SECS
      , cast(substr(variable_info,            -- (3) Query Type
            position(?lqt in variable_info) + char_length(?lqt),
            position(' ' in substr(variable_info, 
                         position(?lqt in variable_info) + char_length(?lqt))))
            as char(30)) QUERY_TYPE
      , cast(substr(variable_info,            -- (4) Query SubType
            position(?lqs in variable_info) + char_length(?lqs),
            position(' ' in substr(variable_info, 
                         position(?lqs in variable_info) + char_length(?lqs))))
            as char(30)) QUERY_SUBTYPE
      , cast(substr(variable_info,            -- (5) Query StatsType
            position(?lsts in variable_info) + char_length(?lsts),
            position(' ' in substr(variable_info, 
                        position(?lsts in variable_info) + char_length(?lsts))))
            as char(30)) AS QUERY_STATS_TYPE 
      , cast(substr(variable_info,            -- (6) QID
             position(?lq in variable_info) + char_length(?lq),
             position(' ' in substr(variable_info, 
                          position(?lq in variable_info) + char_length(?lq))))
            as varchar(175) CHARACTER SET UCS2) QUERY_ID
      , cast(substr(variable_info,            -- (7) SQL Source
             position(?lsq in variable_info) + char_length(?lsq),
             char_length(variable_info) - 
                       ( position(?lsq in variable_info) + char_length(?lsq) ))
            as char(256) CHARACTER SET UCS2) SOURCE_TEXT 
from table (statistics(NULL,?filter)) 
order by 2 descending;
