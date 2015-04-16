#!/usr/bin/env perl

# Example usage:
# PGDATABASE=db1 ./benchmark.pl raw20h64_1000_p3_uncompressed_main

use strict;

@ARGV || die "Usage: $0 <tabname>[:<colname>] ...\n";

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

foreach $a (@ARGV) {
  my $tn="${a}";
  my $col="p";
  if ( $tn =~ /(.*):(.*)/ ) {
    $col = $2;
    $tn = $1;
  }
  print "\n[Table:$tn Column:$col]\n";
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

  my $info = query(<<"EOF"
select count(*), min(pc_numpoints(\"${col}\")),
max(pc_numpoints(\"${col}\")),
avg(pc_numpoints(\"${col}\")),
sum(pc_numpoints(\"${col}\")),
avg(pc_memsize(\"${col}\")),
array_to_string(array_agg(distinct pc_summary(\"${col}\")::json->>'compr'), ',')
from \"${tn}\"
EOF
  );
  my @info = split '\|', $info;
  #print ' Info: ' . join(',', @info) . "\n";
  print ' Patches: ' . $info[0] . "\n";
  printf " Compression: %s\n", $info[6];
  printf " Average patch size (bytes): %d\n", $info[5];
  printf " Average patch points: %d\n", $info[3];
#  print ' Points: ' . $info[4] . ' ('
#       . join('/', @info[1 .. 3])
#       . ' min/max/avg per patch'
#       . ")\n";

  $info = query("select pg_size_pretty(pg_total_relation_size('${tn}'));\n");
  print ' Total relation size: ' . $info . "\n";


  # Speed tests here

  my $iterations = 10;

  my @fname_vals = ();
  foreach my $fname (@dims_name) {
    push @fname_vals, "('${fname}')";
  }
  my $fname_vals = join ',', @fname_vals;

  # full decompression
  my $sql = <<"EOF";
SELECT PC_FilterEquals(\"${col}\",d,
                       PC_PatchMax(\"${col}\",d))
FROM \"${tn}\", ( values ${fname_vals} ) f(d);
EOF
  #print "SQL: $sql";
  my @time = checkTimes $sql, $iterations;
  print ' Full scan time (ms): '
    . join('/', @time)
    . ' (min/max/avg)'
    . "\n";

  # header only
  $sql = <<"EOF";
SELECT PC_PatchAvg(\"${col}\",d)
FROM \"${tn}\", ( values ${fname_vals} ) f(d);
EOF
  #print "SQL: $sql";
  @time = checkTimes $sql, $iterations;
  print ' Stats scan time (ms): '
    . join('/', @time)
    . ' (min/max/avg)'
    . "\n";
}

print "\n";
