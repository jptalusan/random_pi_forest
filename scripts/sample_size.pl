#!/usr/bin/env perl
#from https://stackoverflow.com/questions/37478287/select-n-random-line-from-a-text-file-cut-from-the-original-and-paste-into-new-f
use warnings;
use strict;

my ($filename, $n) = @ARGV;
$filename
    or die "usage: $0 filename sample_size";

-f $filename
    or die "Invalid filename '$filename'";
chomp(my ($word_count_lines) = `/usr/bin/wc -l $filename`);
my ($lines, undef) = split /\s+/, $word_count_lines;

die "Need to pass in sample size"
    unless $n;
my $sample_size = int $n;

die "Invalid sample size '$n', should in the between [ 0 - $lines ]"
    unless (0 < $sample_size and $sample_size < $lines);

# Pick some random line numbers
my %sample;
while ( keys %sample < $sample_size ) {
    $sample{ 1+int rand $lines }++;
}

open my $fh, $filename
    or die "Unable to open '$filename' for reading : $!";

open my $fh_sample, "> $filename.sample"
    or die "Unable to open '$filename.sample' for writing : $!";
open my $fh_remainder, "> $filename.remainder"
    or die "Unable to open '$filename.remainder' for writing : $!";

my $current_fh;
while (<$fh>) {
    my $line_number = $.;
    $current_fh = $sample{ $line_number } ? $fh_sample : $fh_remainder;
    # Write to correct file
    print $current_fh $_;
}
close $fh
    or die "Unable to finish reading '$filename' : $!";
close $fh_sample
    or die "Unable to finish writing '$filename.sample' : $!";
close $fh_remainder
    or die "Unable to finish writing '$filename.sample' : $!";

print "Original file '$filename' has $lines rows\n";
print "Created '$filename.sample' with $sample_size rows\n";
print "Created '$filename.remainder' with " . ($lines - $sample_size) . " rows\n";
print "Run 'mv $filename.remainder $filename' if you are happy with this result\n";
