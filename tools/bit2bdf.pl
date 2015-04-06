#!/usr/local/bin/perl -p
# vi:set ts=8 sts=4 sw=4 tw=0:

if (/^[-.@#]+$/)
{
    chomp;
    y/-#.@/0101/;
    $_ = unpack("H*", pack("B*", $_)) . "\n";
}
