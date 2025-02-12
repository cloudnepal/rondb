--source include/have_ndb.inc

--echo "Create tables for backup"
--disable_query_log
use test;

create table t0(id int primary key, val int) engine=ndb;
create table t1(val int) engine=ndb;
create table t2(id int primary key) engine=ndb;
create table t3(id int primary key auto_increment, val int) engine=ndb;
create table t4(id int primary key, val int default 11001) engine=ndb;
create table t5(id int primary key, val text) engine=ndb;
create table t6(id int primary key, val int unique) engine=ndb;
create table t7(id int auto_increment, val1 int, val2 int, primary key(id,val1)) engine=ndb;
create table t8(id int primary key, val varbinary(29990)) engine=ndb;
create table t9(
  auto int(5) unsigned NOT NULL auto_increment,
  string char(10) default "hello",
  vstring varchar(10) default "hello",
  tiny tinyint(4) DEFAULT '0' NOT NULL ,
  short smallint(6) DEFAULT '1' NOT NULL ,
  medium mediumint(8) DEFAULT '0' NOT NULL,
  long_int int(11) DEFAULT '0' NOT NULL,
  longlong bigint(13) DEFAULT '0' NOT NULL,
  real_float float(13,1) DEFAULT 0.0 NOT NULL,
  real_double double(16,4),
  real_decimal decimal(16,4),
  utiny tinyint(3) unsigned DEFAULT '0' NOT NULL,
  ushort smallint(5) unsigned zerofill DEFAULT '00000' NOT NULL,
  umedium mediumint(8) unsigned DEFAULT '0' NOT NULL,
  ulong int(11) unsigned DEFAULT '0' NOT NULL,
  ulonglong bigint(13) unsigned DEFAULT '0' NOT NULL,
  bits bit(3),
  options enum('one','two','three') not null,
  flags set('one','two','three') not null,
  date_field date,
  year_field year,
  time_field time,
  date_time datetime,
  PRIMARY KEY (auto),
  KEY (utiny),
  KEY (tiny),
  KEY (short),
  KEY any_name (medium),
  KEY (longlong),
  KEY (real_float),
  KEY (ushort),
  KEY (umedium),
  KEY (ulong),
  KEY (ulonglong,ulong),
  KEY (options,flags)
)engine=ndb;

let $i=9;
# insert 9 rows
while ($i)
{
  eval insert into t0 values($i,$i);
  eval insert into t1 values($i);
  eval insert into t2 values($i);
  eval insert into t3(val) values($i);
  eval insert into t4(id) values($i);
  eval insert into t5 values($i,REPEAT('$i', 4000));
  eval insert into t6 values($i,$i);
  eval insert into t7(val1,val2) values($i,$i);
  eval insert into t8 values($i,REPEAT('$i',29000));
  eval insert into t9 (string,vstring,tiny,short,medium,long_int,longlong,
  real_float,real_double, real_decimal,utiny, ushort, umedium,ulong,ulonglong,
  bits,options,flags,date_field,year_field,time_field,date_time)
  values
  ("$i$i$i$i","$i$i$i$i",-$i,-$i,-$i,-$i,-$i,$i.$i,$i.$i,$i.$i,$i,$i,$i,$i,$i,
   b'001','one','one', '19$i$i-$i-$i','19$i$i','$i:$i:$i','19$i$i-$i-$i $i:$i:$i');
  dec $i;
}
# insert rows with NULL values
  eval insert into t0 values(10,NULL);
  eval insert into t1 values(NULL);
  eval insert into t2 values(10);
  eval insert into t3(val) values(NULL);
  eval insert into t4(id) values(10);
  eval insert into t5 values(10,NULL);
  eval insert into t6 values(10,NULL);
  eval insert into t7(val1,val2) values(10,NULL);
  eval insert into t8 values(10,NULL);
  eval insert into t9 (string,vstring,tiny,short,medium,long_int,longlong,
  real_float,real_double, real_decimal,utiny,ushort,umedium,ulong,ulonglong,
  bits,options,flags,date_field,year_field,time_field,date_time)
  values
  (NULL,NULL,10,10,10,10,10,10.1,NULL,NULL,10,10,10,10,10,
   NULL,'two','two',NULL,NULL,NULL,NULL);

--exec $NDB_MGM -e "all error 10039"

if ($snapshotstart)
{
  --echo "Starting backup as nowait,snapshotstart"
  --source suite/ndb/t/ndb_backup_nowait_snapshotstart_start.inc
}
if (!$snapshotstart)
{
  --echo "Starting backup as nowait"
  --source suite/ndb/t/ndb_backup_nowait_start.inc
}

--echo "Update tables during backup"
--disable_query_log
use test;
update t0 set val = 12345 where val is null;
update t1 set val = 11 where val = 1;
update t2 set id = 22 where id = 2;
update t3 set val = 33 where id = 3;
update t4 set val = 44 where id = 4;
update t5 set val = REPEAT("123  $#",1000) where id = 5;
update t6 set val = 66 where id = 6;
update t7 set val1 = 77 where id = 7;
update t8 set val = REPEAT("8", 29900) where id = 8;
update t9 set string="bbbb",vstring="bbbb",tiny=-22,short=-22,medium=-22,
long_int=-22,longlong=-22,real_float=22.22,real_double=22.22,real_decimal=22.22,
utiny=22,ushort=22,umedium=22,ulong=22,ulonglong=22, bits=b'010',
options='two',flags='two', date_field='1922-02-22',year_field='1922',
time_field='22:22:22',date_time='1922-02-22 22:22:22' where auto=1;
--enable_query_log

--exec $NDB_MGM -e "all error 0"

--echo "Wait for backup to complete"
--source suite/ndb/t/ndb_backup_nowait_wait.inc

--echo "Drop tables after backup completes"
--disable_query_log
use test;
drop table t0;
drop table t1;
drop table t2;
drop table t3;
drop table t4;
drop table t5;
drop table t6;
drop table t7;
drop table t8;
drop table t9;

--source include/ndb_backup_id.inc
--enable_query_log

if (!$serialrestore)
{
  --echo "Restoring metadata"
  --exec $NDB_RESTORE -b $the_backup_id -n 1 -m $NDB_BACKUPS-$the_backup_id/ >> $NDB_TOOLS_OUTPUT 2>&1

  --echo "Restoring backup on node 1"
  --exec $NDB_RESTORE --allow-unique-indexes -b $the_backup_id -n 1 -r $NDB_BACKUPS-$the_backup_id/ >> $NDB_TOOLS_OUTPUT 2>&1

  --echo "Restoring backup on node 2"
  --exec $NDB_RESTORE --allow-unique-indexes -b $the_backup_id -n 2 -r $NDB_BACKUPS-$the_backup_id/ >> $NDB_TOOLS_OUTPUT 2>&1
}
if ($serialrestore)
{
  --echo "Restoring metadata"
  --exec $NDB_RESTORE -b $the_backup_id -n 1 -m $NDB_BACKUPS-$the_backup_id/BACKUP-$the_backup_id-PART-1-OF-$backup_parts >> $NDB_TOOLS_OUTPUT 2>&1

  --echo "Restoring multithreaded backup parts serially on node 1"
  let $current_part = $backup_parts;
  while ($current_part)
  {
    --exec $NDB_RESTORE --allow-unique-indexes -b $the_backup_id -n 1 -r $NDB_BACKUPS-$the_backup_id/BACKUP-$the_backup_id-PART-$current_part-OF-$backup_parts >> $NDB_TOOLS_OUTPUT 2>&1
    dec $current_part;
  }

  --echo "Restoring multithreaded backup parts serially on node 2"
  let $current_part = $backup_parts;
  while ($current_part)
  {
    --exec $NDB_RESTORE --allow-unique-indexes -b $the_backup_id -n 2 -r $NDB_BACKUPS-$the_backup_id/BACKUP-$the_backup_id-PART-$current_part-OF-$backup_parts >> $NDB_TOOLS_OUTPUT 2>&1
    dec $current_part;
  }
}
--echo "Print restored rows"
use test;
select * from t0 order by id;
select * from t1 order by val;
select * from t2 order by id;
select * from t3 order by id;
select * from t4 order by id;
select id, sha1(val) from t5 order by id;
select * from t6 order by id;
select * from t7 order by id;
select id, sha1(val) from t8 order by id;
select * from t9 order by auto;

--echo "Drop tables after backup completes"
--disable_query_log
use test;
drop table t0;
drop table t1;
drop table t2;
drop table t3;
drop table t4;
drop table t5;
drop table t6;
drop table t7;
drop table t8;
drop table t9;
--enable_query_log
