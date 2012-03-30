#!/usr/bin/perl
use strict;
use warnings;
use LWP::UserAgent;
use HTTP::Request::Common qw(GET);

my $proxy_url = 'http://houndshark.ics.cs.cmu.edu:54538';
my $proxy_agent = LWP::UserAgent->new;
$proxy_agent->proxy(['http'], $proxy_url);

my @test_urls = ( 'http://www.cmu.edu', 'http://www.cs.cmu.edu/~213',
  'http://www.cnn.com', 'http://www.youtube.com', 'http://www.nytimes.com');

my $count;
for ($count = 0; $count < 10; $count++){
  sanity_test();
  cache_test();
}



sub sanity_test {

  my $test_url;
  # Check list of URLs for successful response
  foreach $test_url (@test_urls) {
    my $req = GET $test_url;

    # Make the request
    my $res = $proxy_agent->request($req);

    # Check the response
    unless ($res->is_success) {
      print $test_url . ": ";
      print $res->status_line . "\n";
    }
  }
}

#
# Goes to basic website that displays time as an html document.
# Checks that the cached time is stored and a new time is not recieved
#
sub cache_test {
  my $req1 = GET 'http://tycho.usno.navy.mil/cgi-bin/timer.pl';
  my $res1 = $proxy_agent->request($req1);
  
  my $req2 = GET 'http://tycho.usno.navy.mil/cgi-bin/timer.pl';
  my $res2 = $proxy_agent->request($req2);

  unless ($res1->content eq $res2->content) {
    print "FAILED: Cache not working";
  }

}

exit 0;
