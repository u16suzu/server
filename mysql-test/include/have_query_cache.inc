# Tests will be skipped for the view protocol because the view protocol creates 
# an additional util connection and other statistics data
-- source include/no_view_protocol.inc

-- require include/have_query_cache.require
disable_query_log;
show variables like 'have_query_cache';
enable_query_log;
