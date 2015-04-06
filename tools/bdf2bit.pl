#!/usr/local/bin/perl -p
# vi:set ts=8 sts=4 sw=4 tw=0:

if (/^[0-9a-fA-F]+$/)
{
    chomp;
    $_ = unpack("B*", pack("H*", $_)) . "\n";
    y/01/.@/;
}
