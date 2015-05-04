#!/usr/bin/env perl

# Example usage:
# PGDATABASE=db1 ./benchmark.pl raw20h64_1000_p3_uncompressed_main

use strict;

sub usage {
  my $s = <<"EOF";
Usage: $0 [<options>] <tabname>[:<colname>] ...
Options:
  --query <query>  A query to run. Can be specified multiple times.
                   The `:c' string will be replaced with the column name.
                   A default set of queries are run if none are provided.
  --iterate <n>    Number of times to run each query, defaults to 1.
EOF
  return $s;
}

my @QUERIES = ();
my $iterations = 1; # this one might be unneeded


# Parse commandline arguments
for (my $i=0; $i<@ARGV; ++$i) {
  if ( $ARGV[$i] =~ /^--/ ) {
    my $switch = splice @ARGV, $i, 1;
    if ( $switch eq '--query' ) {
      my $query = splice @ARGV, $i, 1;
      --$i; # rewind as argv shrinked
      push @QUERIES, $query;
    } elsif ( $switch eq '--iterate' ) {
      $iterations = splice @ARGV, $i, 1;
      --$i; # rewind as argv shrinked
    } else {
      die "Unrecognized option $switch\n";
    }
  }
}

@ARGV || die usage;

my $PSQL = "psql";
my $PSQL_OPTS = '-tXA';

sub query {
  my $sql = shift;
  open OUTPUT, "-|", ${PSQL}, ${PSQL_OPTS}, '-c', ${sql} || die "Cannot run ${PSQL}";
  my @out = <OUTPUT>;
  close OUTPUT;
  my $ret = join '', @out;
  chop $ret;
  return $ret;
}

# checkTimes <sql> <iterations>
# Check min/max/avg times of running <sql> over <iterations> iterations
sub checkTimes {
  my $sql = shift;
  my $iterations = shift;
  my $min = 1e100; # TODO: use highest number
  my $max = 0;
  my $sum = 0;
  my $i;

  $sql = "explain analyze $sql";
  for (my $i=0; $i<$iterations; $i++)
  {
    my $out = query($sql);
    #print $out;
    $out =~ /Total runtime: ([0-9\.]*) ms/m
      || die "Could not extract runtime info, output is:\n$out\n";
    my $time = 0+$1;
    $min = $time if $time < $min;
    $max = $time if $time > $max;
    $sum += $time;
    #print " runtime in ms: $time, min:$min, max:$max\n";
    # TODO: parse "Total Runtime"
    # Example:
    # Total runtime: 2.432 ms
  }
  my $avg = $sum / $iterations;
  return ($min,$max,$avg);
}

# reportTimes @times, iterations
# Check min/max/avg times of running <sql> over <iterations> iterations
sub reportTimes {
  my $label = shift;
  my $sql = shift;
  my $iterations = shift;

  my @time = checkTimes($sql, $iterations);

  my $s = $label . ': ';
  if ( $iterations > 1 ) {
    $s .= join(' / ', @time);
  } else {
    $s .= $time[0];
  }
  return $s;
}

# Default queries
if ( ! @QUERIES ) {
  push @QUERIES, (
    # Header scan
    'PC_Envelope(:c)',
    # Decompression
    'PC_Uncompress(:c)'
    # Full points scan
    ,'PC_Explode(:c)'
    # Conversion to JSON (needed?)
    ,'PC_AsText(:c)'
   );
}

foreach $a (@ARGV) {
  my $tn="${a}";
  my $col="pa";
  if ( $tn =~ /(.*):(.*)/ ) {
    $col = $2;
    $tn = $1;
  }
  print "\n[$tn:$col]\n";

  my $info = query(<<"EOF"
select pg_size_pretty(pg_relation_size('${tn}')), -- main
       -- toasts
       pg_size_pretty(pg_table_size('${tn}')-pg_relation_size('${tn}')),
       -- indexes
       pg_size_pretty(pg_total_relation_size('${tn}')-pg_table_size('${tn}')),
       -- total
       pg_size_pretty(pg_total_relation_size('${tn}'))
EOF
  );
  my @info = split '\|', $info;

  print ' Relation size: ' . $info[0] . ' + ' . $info[1]
      . ' + ' . $info[2] . ' = ' . $info[3] . " (M+T+I)\n";

  $info = query(<<"EOF"
SELECT CASE WHEN attstorage = 'm' THEN 'main'
            WHEN attstorage = 'e' THEN 'external'
            WHEN attstorage = 'p' THEN 'plain'
            WHEN attstorage = 'x' THEN 'extended'
            ELSE attstorage::text
       END
FROM pg_attribute
WHERE attrelid = '${tn}'::regclass::oid
  AND attname = '${col}'
EOF
  );
  print ' Patch column storage: ' . $info . "\n";

  my $dims = query(<<"EOF"
with format as (
 select f."schema" s from pointcloud_formats f, pointcloud_columns c
where "table"='${tn}' and "column" = '${col}'
 and f.pcid = c.pcid
), meta0 as (
  select s, (regexp_matches(s, 'pc="([^"]*)"'))[1] nsp
  from format
), meta as (
  select s, ARRAY[ARRAY['pc',nsp]] ns
  from meta0
), dims0 as (
  select unnest(xpath('//pc:dimension', s::xml, ns)) d
  from meta
), ordered as (
  select d from dims0, meta
  order by (xpath('//pc:position/text()', d, ns))[1]::text::int
), dims as (
--<pc:dimension xmlns:pc="http://pointcloud.org/schemas/PC/1.1">
--  <pc:position>13</pc:position>
--  <pc:size>8</pc:size>
--  <pc:name>GPSTime</pc:name>
--  <pc:interpretation>double</pc:interpretation>
--</pc:dimension>
  select
    (xpath('//pc:name/text()', d, ns))[1] || ':' ||
    replace((xpath('//pc:interpretation/text()', d, ns))[1]::text, '_t', '')
    d
  from ordered, meta
)
select array_to_string(array_agg(d),',') from dims
EOF
  );
  my @dims_interp = ();
  my @dims_name = ();
  foreach my $dim (split ',', $dims) {
    my ($name, $interp) = split(':', $dim);
    push @dims_name, $name;
    push @dims_interp, $interp;
  }
  print " Dims: " . join(',',@dims_interp) . "\n";

  $info = query(<<"EOF"
select count(*), -- 0
min(pc_numpoints(\"${col}\")), -- 1
max(pc_numpoints(\"${col}\")), -- 2
avg(pc_numpoints(\"${col}\")), -- 3
sum(pc_numpoints(\"${col}\")), -- 4
avg(pc_memsize(\"${col}\")), -- 5
array_to_string(array_agg(distinct pc_summary(\"${col}\")::json->>'compr'), ','),
pg_size_pretty(sum(pg_column_size(\"${col}\")))  -- 7
from \"${tn}\"
EOF
  );

  @info = split '\|', $info;
  #print ' Info: ' . join(',', @info) . "\n";
  print ' Total patch column size: ' . $info[7] . "\n";
  print ' Patches: ' . $info[0] . "\n";
  printf "  Compression: %s\n", $info[6];
  printf "  Average patch size (bytes): %d\n", $info[5];
  printf "  Average patch points: %d\n", $info[3];
#  print ' Points: ' . $info[4] . ' ('
#       . join('/', @info[1 .. 3])
#       . ' min/max/avg per patch'
#       . ")\n";


  #$info = query("select pg_size_pretty(pg_total_relation_size('${tn}'));\n");
  #print ' Total relation size: ' . $info . "\n";

  #next; # TODO: allow early exit here, before running speed tests


  # Speed tests here

  print " Timings ";
  if ( $iterations > 1 ) {
      print "(min/max/avg ms over ${iterations} iterations):\n";
  } else {
    print "(ms):\n";
  }

  for my $query ( @QUERIES )
  {
    my $sql = $query;
    $sql =~ s/:c/"${col}"/g;
    if ( $sql =~ /\s*SELECT/i ) {
      $sql =~ s/:t/"${tn}"/g;
    } else {
      $sql = "SELECT ${sql} FROM \"${tn}\"";
      # TODO: add where clause if requested
    }
    my $lbl = $sql;
    $lbl =~ s/.*SELECT *([a-zA-Z_]*).*/\1/i;
    #print "LBL: $lbl --- SQL: $sql";
    print '  ' . reportTimes($lbl, $sql, $iterations) . "\n";
  }
}

print "\n";
