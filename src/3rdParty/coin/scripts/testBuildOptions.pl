#!/usr/bin/perl

use Expect;
use IO::Handle;
use File::Path;

$ticker_mode=2;
$makeOpts="-j4";
$,="\n";
$timeout=.01;

#Usage testBuildOptions.pl [buildDir] [sourceDir]

$0=~/(.*?)\/?([^\/]*?)(\.pl)?$/;
$myName=$2;
$myDir=$1;

use Cwd;
chdir $myDir;
$myDir=Cwd::getcwd;

$buildDir=$ARGV[0];
$srcDir=$ARGV[1];

$buildDir="/tmp/${myName}/current" if (!${buildDir});
$srcDir="${myDir}/.." if (!${srcDir});


open(INFILE,"<${srcDir}/configure.ac");
local $/;
my $confContent=<INFILE>;
close(INFILE);

@enableCmds=get("AC_ARG_ENABLE", $confContent);
#@withCmds=get("AC_ARG_WITH", $confContent);

my @cmds="";
for ($i=0;$i<=$#enableCmds;++$i) {
    push @cmds, "--enable-$enableCmds[$i]";
    push @cmds, "--disable-$enableCmds[$i]";
}

#for ($i=0;$i<=$#withCmds;++$i) {
#    push @cmds, "--with-$withCmds[$i]\n";
#}

my @results;
for(my $i=0; $i<=$#cmds; ++$i) {
    printf "======= %3u of %3u ======\n=", $i+1, $#cmds+1;
    my $res=build($srcDir,$buildDir, $cmds[$i]);
    push @results, $res;
    saveBuild($buildDir, $cmds[$i]) if ($res);
}

local $\="\n";
for ($j=0;$j<=3;++$j) {
    print "Success:" if ($j==0);
    print "Failed during configure:" if ($j==1);
    print "Failed during make:" if ($j==2);
    print "Failed during testsuite:" if ($j==3);
    for($i=0;$i<=$#results;$i++) 
    {
	print "\t\"$cmds[$i]\"" if ($results[$i]==$j);
    }
}

sub sane($) {
    my $string=$_[0];
    $string=~s/\s//g;
    $string=~s/\-+//g;
    $string.="None" if ($string=~/\/$/);
    return $string;
}

sub saveBuild($$) {
    my $oldDir=$_[0];
    my $name=$_[1];
    $oldDir=~/(.*)?\/[^\/]+$/;
    $newDir="$1/$name";
    $newDir=sane($newDir);
    rename $oldDir, $newDir;
}

sub log10($) {
    return log($_[0])/log(10);
}

sub numChars($) {
    return split(//,$_[0]);
}

sub eraseFor($) {
    my $val=$_[0];
    my $n=numChars($val);
    my $retVal="";
    for $i (1..$n) {
	$retVal=$retVal."\b";
    }
    return $retVal;
}

sub get($$) {
    my $command=$_[0];
    my $rest=$_[1];
    my @retVal;
    while ($rest=~/${command}\((.*)/s)
    {
	$rest=$1;
	$1=~/([^,]*)(.*)/;
	$token=$1;
	chomp($token);
	$token=~s/^\s*\[//s;
	    $token=~s/\]\s*$//s;
	    push @retVal, $token;
    }
    return @retVal;
}

sub build($$) {
    my $srcDir=$_[0];
    shift;
    my $buildDir=$_[0];
    shift;
    print "$srcDir\n";
    print "$buildDir\n";

    mkpath $buildDir || die "Could not create $buildDir";

    chdir ${buildDir} && system2("rm -rf *", "Removing old build", "rm") && die "Could not enter $buildDir";
    system2("${srcDir}/configure @_", "Running configure @_", "configure") == 0 || return 1;
    system2("make ${makeOpts}", "Running make", "make") == 0 || return 2;
    chdir "testsuite";
    system2("make ${makeOpts}", "Running testsuite", "testsuite") == 0 || return 3;

    return 0;
}



sub start_ticker($) {
    my $id=$_[0];
    $ticker_count{$id}=0;
    $d=$ticker_total{$id};
    if (!defined($d)) {
	    $ticker_total{$id} = 0;
	    $d=0;
	    $ticker_uses{$id} = 0;
	}
	$ticker_uses{$id}++;
	
    print "$ticker_count{$id}/$d";
    sleep 1 ;
}

sub update_ticker($$) {
    my @states=("/","-","\\","|");
    if ($_[0]) {
	    if ($ticker_mode == 0) {
		    print $_[0];
		}
		elsif ($ticker_mode == 1) {
		    print ".";
		}
		elsif ($ticker_mode == 2) {
		    my $id=$_[1];
		    print eraseFor($ticker_count{$id});
		    $d=$ticker_total{$id};
		    print eraseFor(1);
		    print eraseFor($d);
		    $text=$_[0];
		    my $n=split(/\n/,$text);
		    $ticker_count{$id}+=$n;
		    print "$ticker_count{$id}/$d";
		}
		elsif ($ticker_mode == 3) {
		    print "\b";
		    print $states[++$ticker_count%4];
		}
	}
}

sub finish_ticker($) {
    if ($ticker_mode==3) {
	    print "\b";
	}
	elsif ($ticker_mode > 0) {
	    if ($ticker_mode==2) {
		    my $id=$_[0];
		    $d=$ticker_total{$id};
		    if ($d>1) {
			    $ticker_total{$id}=int($ticker_total{$id}*($ticker_uses{$id}-1.0)/$ticker_uses{$id} + $ticker_count{$id}/(1.0*$ticker_uses{$id})+0.5);
			    
			}
			else {
			    $ticker_total{$id}=$ticker_count{$id};
			}
		}
	    print "\n";
	}
}

sub system2($$$) {
    print "$_[1]\n";
    $identifier=$_[2];
    my @args=split (/ /,$_[0]);
    my $cmd=shift @args;
    select STDOUT; $|=1;
    my $out = new Expect;
    $out->spawn($cmd,@args);
    $out->log_stdout(0);
    $out->slave->stty(qw(raw -echo));
    my $error = 1;
    start_ticker($identifier);
    while ($error == 1) {
	(my $mp, $error, my $ms, my $before, my $after)=$out->expect($timeout);
	$out->clear_accum();
	update_ticker($before,$identifier);
	sleep $timeout;
    } 
    finish_ticker($identifier);

    $es = $out->exitstatus();
    $out->do_soft_close();
#    die "Exit status $es" if ($es!=0);
    return $es;
}
