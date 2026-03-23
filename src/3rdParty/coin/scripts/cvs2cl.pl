#!/bin/sh
exec perl -w -x $0 ${1+"$@"} # -*- mode: perl; perl-indent-level: 2; -*-
#!perl -w

##############################################################
###                                                        ###
### cvs2cl.pl: produce ChangeLog(s) from `cvs log` output. ###
###                                                        ###
##############################################################

## $Revision: 2.29 $
## $Date: 2000/09/06 16:57:18 $
## $Author: kfogel $
##
##   (C) 1999 Karl Fogel <kfogel@red-bean.com>, under the GNU GPL.
##
##   (Extensively hacked on by Melissa O'Neill <oneill@cs.sfu.ca>.)
##
## cvs2cl.pl is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## cvs2cl.pl is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You may have received a copy of the GNU General Public License
## along with cvs2cl.pl; see the file COPYING.  If not, write to the
## Free Software Foundation, Inc., 59 Temple Place - Suite 330,
## Boston, MA 02111-1307, USA.



use strict;
use Text::Wrap;
use Time::Local;
use File::Basename;


# The Plan:
#
# Read in the logs for multiple files, spit out a nice ChangeLog that
# mirrors the information entered during `cvs commit'.
#
# The problem presents some challenges. In an ideal world, we could
# detect files with the same author, log message, and checkin time --
# each <filelist, author, time, logmessage> would be a changelog entry.
# We'd sort them; and spit them out.  Unfortunately, CVS is *not atomic*
# so checkins can span a range of times.  Also, the directory structure
# could be hierarchical.
#
# Another question is whether we really want to have the ChangeLog
# exactly reflect commits. An author could issue two related commits,
# with different log entries, reflecting a single logical change to the
# source. GNU style ChangeLogs group these under a single author/date.
# We try to do the same.
#
# So, we parse the output of `cvs log', storing log messages in a
# multilevel hash that stores the mapping:
#   directory => author => time => message => filelist
# As we go, we notice "nearby" commit times and store them together
# (i.e., under the same timestamp), so they appear in the same log
# entry.
#
# When we've read all the logs, we twist this mapping into
# a time => author => message => filelist mapping for each directory.
#
# If we're not using the `--distributed' flag, the directory is always
# considered to be `./', even as descend into subdirectories.


############### Globals ################


# What we run to generate it:
my $Log_Source_Command = "cvs log";

# In case we have to print it out:
my $VERSION = '$Revision: 2.29 $';
$VERSION =~ s/\S+\s+(\S+)\s+\S+/$1/;

## Vars set by options:

# Print debugging messages?
my $Debug = 0;

# Just show version and exit?
my $Print_Version = 0;

# Just print usage message and exit?
my $Print_Usage = 0;

# Single top-level ChangeLog, or one per subdirectory?
my $Distributed = 0;

# What file should we generate (defaults to "ChangeLog")?
my $Log_File_Name = "ChangeLog";

# Expand usernames to email addresses based on a map file?
my $User_Map_File = "";

# Output to a file or to stdout?
my $Output_To_Stdout = 0;

# Eliminate empty log messages?
my $Prune_Empty_Msgs = 0;

# Don't call Text::Wrap on the body of the message
my $No_Wrap = 0;

# Separates header from log message.  Code assumes it is either " " or
# "\n\n", so if there's ever an option to set it to something else,
# make sure to go through all conditionals that use this var.
my $After_Header = " ";

# Format more for programs than for humans.
my $XML_Output = 0;

# Do some special tweaks for log data that was written in FSF
# ChangeLog style.
my $FSF_Style = 0;

# Show times in UTC instead of local time
my $UTC_Times = 0;

# Show day of week in output?
my $Show_Day_Of_Week = 0;

# Show revision numbers in output?
my $Show_Revisions = 0;

# Show tags (symbolic names) in output?
my $Show_Tags = 0;

# Show branches by symbolic name in output?
my $Show_Branches = 0;

# Show only revisions on these branches or their ancestors.
my @Follow_Branches;

# Don't bother with files matching this regexp.
my @Ignore_Files;

# How exactly we match entries.  We definitely want "o",
# and user might add "i" by using --case-insensitive option.
my $Case_Insensitive = 0;

# Maybe only show log messages matching a certain regular expression.
my $Regexp_Gate = "";

# Pass this global option string along to cvs, to the left of `log':
my $Global_Opts = "";

# Pass this option string along to the cvs log subcommand:
my $Command_Opts = "";

# Read log output from stdin instead of invoking cvs log?
my $Input_From_Stdin = 0;

# Don't show filenames in output.
my $Hide_Filenames = 0;

# Max checkin duration. CVS checkin is not atomic, so we may have checkin
# times that span a range of time. We assume that checkins will last no
# longer than $Max_Checkin_Duration seconds, and that similarly, no
# checkins will happen from the same users with the same message less
# than $Max_Checkin_Duration seconds apart.
my $Max_Checkin_Duration = 180;

# What to put at the front of [each] ChangeLog.
my $ChangeLog_Header = "";

## end vars set by options.

# In 'cvs log' output, one long unbroken line of equal signs separates
# files:
my $file_separator = "======================================="
                   . "======================================";

# In 'cvs log' output, a shorter line of dashes separates log messages
# within a file:
my $logmsg_separator = "----------------------------";


############### End globals ############




&parse_options ();
&derive_change_log ();



### Everything below is subroutine definitions. ###

# Fills up a ChangeLog structure in the current directory.
sub derive_change_log ()
{
  # See "The Plan" above for a full explanation.

  my %grand_poobah;

  my $file_full_path;
  my $time;
  my $revision;
  my $author;
  my $msg_txt;
  my $detected_file_separator;

  # We might be expanding usernames
  my %usermap;

  # In general, it's probably not very maintainable to use state
  # variables like this to tell the loop what it's doing at any given
  # moment, but this is only the first one, and if we never have more
  # than a few of these, it's okay.
  my $collecting_symbolic_names = 0;
  my %symbolic_names;    # Where tag names get stored.
  my %branch_names;      # We'll grab branch names while we're at it.
  my %branch_numbers;    # Save some revisions for @Follow_Branches
  my @branch_roots;      # For showing which files are branch ancestors.

  # Bleargh.  Compensate for a deficiency of custom wrapping.
  if (($After_Header ne " ") and $FSF_Style)
  {
    $After_Header .= "\t";
  }

  if (! $Input_From_Stdin) {
    open (LOG_SOURCE, "$Log_Source_Command |")
        or die "unable to run \"${Log_Source_Command}\"";
  }
  else {
    open (LOG_SOURCE, "-") or die "unable to open stdin for reading";
  }

  %usermap = &maybe_read_user_map_file ();

  while (<LOG_SOURCE>)
  {
    # If on a new file and don't see filename, skip until we find it, and
    # when we find it, grab it.
    if ((! (defined $file_full_path)) and /^Working file: (.*)/)
    {
      $file_full_path = $1;
      if (@Ignore_Files)
      {
        my $base;
        ($base, undef, undef) = fileparse ($file_full_path);
        # Ouch, I wish trailing operators in regexps could be
        # evaluated on the fly!
        if ($Case_Insensitive) {
          if (grep ($file_full_path =~ m|$_|i, @Ignore_Files)) {
            undef $file_full_path;
          }
        }
        elsif (grep ($file_full_path =~ m|$_|, @Ignore_Files)) {
          undef $file_full_path;
        }
      }
      next;
    }

    # Just spin wheels if no file defined yet.
    next if (! $file_full_path);

    # Collect tag names in case we're asked to print them in the output.
    if (/^symbolic names:$/) {
      $collecting_symbolic_names = 1;
      next;  # There's no more info on this line, so skip to next
    }
    if ($collecting_symbolic_names)
    {
      # All tag names are listed with whitespace in front in cvs log
      # output; so if see non-whitespace, then we're done collecting.
      if (/^\S/) {
        $collecting_symbolic_names = 0;
      }
      else    # we're looking at a tag name, so parse & store it
      {
        # According to the Cederqvist manual, in node "Tags", tag
        # names must start with an uppercase or lowercase letter and
        # can contain uppercase and lowercase letters, digits, `-',
        # and `_'.  However, it's not our place to enforce that, so
        # we'll allow anything CVS hands us to be a tag:
        /^\s+([^:]+): ([\d.]+)$/;
        my $tag_name = $1;
        my $tag_rev  = $2;

        # A branch number either has an odd number of digit sections
        # (and hence an even number of dots), or has ".0." as the
        # second-to-last digit section.  Test for these conditions.
        my $real_branch_rev = "";
        if (($tag_rev =~ /^(\d+\.\d+\.)+\d+$/)   # Even number of dots...
            and (! ($tag_rev =~ /^(1\.)+1$/)))   # ...but not "1.[1.]1"
        {
          $real_branch_rev = $tag_rev;
        }
        elsif ($tag_rev =~ /(\d+\.(\d+\.)+)0.(\d+)/)  # Has ".0."
        {
          $real_branch_rev = $1 . $3;
        }
        # If we got a branch, record its number.
        if ($real_branch_rev)
        {
          $branch_names{$real_branch_rev} = $tag_name;
          if (@Follow_Branches) {
            if (grep ($_ eq $tag_name, @Follow_Branches)) {
              $branch_numbers{$tag_name} = $real_branch_rev;
            }
          }
        }
        else {
          # Else it's just a regular (non-branch) tag.
          push (@{$symbolic_names{$tag_rev}}, $tag_name);
        }
      }
    }
    # End of code for collecting tag names.

    # If have file name, but not revision, and see revision, then grab
    # it.  (We collect unconditionally, even though we may or may not
    # ever use it.)
    if ((! (defined $revision)) and (/^revision (\d+\.[\d.]+)/))
    {
      $revision = $1;

      if (@Follow_Branches)
      {
        foreach my $branch (@Follow_Branches)
        {
          # Special case for following trunk revisions
          if (($branch =~ /^trunk$/i) and ($revision =~ /^[0-9]+\.[0-9]+$/))
          {
            goto dengo;
          }

          my $branch_number = $branch_numbers{$branch};
          if ($branch_number)
          {
            # Are we on one of the follow branches or an ancestor of
            # same?
            #
            # If this revision is a prefix of the branch number, or
            # possibly is less in the minormost number, OR if this
            # branch number is a prefix of the revision, then yes.
            # Otherwise, no.
            #
            # So below, we determine if any of those conditions are
            # met.

            # Trivial case: is this revision on the branch?
            # (Compare this way to avoid regexps that screw up Emacs
            # indentation, argh.)
            if ((substr ($revision, 0, ((length ($branch_number)) + 1)))
                eq ($branch_number . "."))
            {
              goto dengo;
            }
            # Non-trivial case: check if rev is ancestral to branch
            elsif ((length ($branch_number)) > (length ($revision)))
            {
              $revision =~ /^((?:\d+\.)+)(\d+)$/;
              my $r_left = $1;          # still has the trailing "."
              my $r_end = $2;

              $branch_number =~ /^((?:\d+\.)+)(\d+)\.\d+$/;
              my $b_left = $1;  # still has trailing "."
              my $b_mid  = $2;   # has no trailing "."

              if (($r_left eq $b_left)
                  && ($r_end <= $b_mid))
              {
                goto dengo;
              }
            }
          }
        }
      }
      else    # (! @Follow_Branches)
      {
        next;
      }

      # Else we are following branches, but this revision isn't on the
      # path.  So skip it.
      undef $revision;
    dengo:
      next;
    }

    # If we don't have a revision right now, we couldn't possibly
    # be looking at anything useful.
    if (! (defined ($revision))) {
      $detected_file_separator = /^$file_separator$/o;
      if ($detected_file_separator) {
        # No revisions for this file; can happen, e.g. "cvs log -d DATE"
        goto CLEAR;
      }
      else {
        next;
      }
    }

    # If have file name but not date and author, and see date or
    # author, then grab them:
    unless (defined $time)
    {
      if (/^date: .*/)
      {
        ($time, $author) = &parse_date_and_author ($_);
        if (defined ($usermap{$author}) and $usermap{$author}) {
          $author = $usermap{$author};
        }
      }
      else {
        $detected_file_separator = /^$file_separator$/o;
        if ($detected_file_separator) {
          # No revisions for this file; can happen, e.g. "cvs log -d DATE"
          goto CLEAR;
        }
      }
      # If the date/time/author hasn't been found yet, we couldn't
      # possibly care about anything we see.  So skip:
      next;
    }

    # A "branches: ..." line here indicates that one or more branches
    # are rooted at this revision.  If we're showing branches, then we
    # want to show that fact as well, so we collect all the branches
    # that this is the latest ancestor of and store them in
    # @branch_roots.  Just for reference, the format of the line we're
    # seeing at this point is:
    #
    #    branches:  1.5.2;  1.5.4;  ...;
    #
    # Okay, here goes:

    if (/^branches:\s+(.*);$/)
    {
      if ($Show_Branches)
      {
        my $lst = $1;
        $lst =~ s/(1\.)+1;|(1\.)+1$//;  # ignore the trivial branch 1.1.1
        if ($lst) {
          @branch_roots = split (/;\s+/, $lst);
        }
        else {
          undef @branch_roots;
        }
        next;
      }
      else
      {
        # Ugh.  This really bothers me.  Suppose we see a log entry
        # like this:
        #
        #    ----------------------------
        #    revision 1.1
        #    date: 1999/10/17 03:07:38;  author: jrandom;  state: Exp;
        #    branches:  1.1.2;
        #    Intended first line of log message begins here.
        #    ----------------------------
        #
        # The question is, how we can tell the difference between that
        # log message and a *two*-line log message whose first line is
        #
        #    "branches:  1.1.2;"
        #
        # See the problem?  The output of "cvs log" is inherently
        # ambiguous.
        #
        # For now, we punt: we liberally assume that people don't
        # write log messages like that, and just toss a "branches:"
        # line if we see it but are not showing branches.  I hope no
        # one ever loses real log data because of this.
        next;
      }
    }

    # If have file name, time, and author, then we're just grabbing
    # log message texts:
    $detected_file_separator = /^$file_separator$/o;
    if ($detected_file_separator && ! (defined $revision)) {
      # No revisions for this file; can happen, e.g. "cvs log -d DATE"
      goto CLEAR;
    }
    unless ($detected_file_separator || /^$logmsg_separator$/o)
    {
      $msg_txt .= $_;   # Normally, just accumulate the message...
      next;
    }
    # ... until a msg separator is encountered:
    # Ensure the message contains something:
    if ((! $msg_txt)
        || ($msg_txt =~ /^\s*\.\s*$|^\s*$/)
        || ($msg_txt =~ /\*\*\* empty log message \*\*\*/))
    {
      if ($Prune_Empty_Msgs) {
        goto CLEAR;
      }
      # else
      $msg_txt = "[no log message]\n";
    }

    ### Store it all in the Grand Poobah:
    {
      my $dir_key;        # key into %grand_poobah
      my %qunk;           # complicated little jobbie, see below

      # Each revision of a file has a little data structure (a `qunk')
      # associated with it.  That data structure holds not only the
      # file's name, but any additional information about the file
      # that might be needed in the output, such as the revision
      # number, tags, branches, etc.  The reason to have these things
      # arranged in a data structure, instead of just appending them
      # textually to the file's name, is that we may want to do a
      # little rearranging later as we write the output.  For example,
      # all the files on a given tag/branch will go together, followed
      # by the tag in parentheses (so trunk or otherwise non-tagged
      # files would go at the end of the file list for a given log
      # message).  This rearrangement is a lot easier to do if we
      # don't have to reparse the text.
      #
      # A qunk looks like this:
      #
      #   {
      #     filename    =>    "hello.c",
      #     revision    =>    "1.4.3.2",
      #     time        =>    a timegm() return value (moment of commit)
      #     tags        =>    [ "tag1", "tag2", ... ],
      #     branch      =>    "branchname" # There should be only one, right?
      #     branchroots =>    [ "branchtag1", "branchtag2", ... ]
      #   }

      if ($Distributed) {
        # Just the basename, don't include the path.
        ($qunk{'filename'}, $dir_key, undef) = fileparse ($file_full_path);
      }
      else {
        $dir_key = "./";
        $qunk{'filename'} = $file_full_path;
      }

      # This may someday be used in a more sophisticated calculation
      # of what other files are involved in this commit.  For now, we
      # don't use it, because the common-commit-detection algorithm is
      # hypothesized to be "good enough" as it stands.
      $qunk{'time'} = $time;

      # We might be including revision numbers and/or tags and/or
      # branch names in the output.  Most of the code from here to
      # loop-end deals with organizing these in qunk.

      $qunk{'revision'} = $revision;

      # Grab the branch, even though we may or may not need it:
      $qunk{'revision'} =~ /((?:\d+\.)+)\d+/;
      my $branch_prefix = $1;
      $branch_prefix =~ s/\.$//;  # strip off final dot
      if ($branch_names{$branch_prefix}) {
        $qunk{'branch'} = $branch_names{$branch_prefix};
      }

      # If there's anything in the @branch_roots array, then this
      # revision is the root of at least one branch.  We'll display
      # them as branch names instead of revision numbers, the
      # substitution for which is done directly in the array:
      if (@branch_roots) {
        my @roots = map { $branch_names{$_} } @branch_roots;
        $qunk{'branchroots'} = \@roots;
      }

      # Save tags too.
      if (defined ($symbolic_names{$revision})) {
        $qunk{'tags'} = $symbolic_names{$revision};
        delete $symbolic_names{$revision};
      }

      # Add this file to the list
      # (We use many spoonfuls of autovivication magic. Hashes and arrays
      # will spring into existence if they aren't there already.)

      &debug ("(pushing log msg for ${dir_key}$qunk{'filename'})\n");

      # Store with the files in this commit.  Later we'll loop through
      # again, making sure that revisions with the same log message
      # and nearby commit times are grouped together as one commit.
      push (@{$grand_poobah{$dir_key}{$author}{$time}{$msg_txt}}, \%qunk);
    }

  CLEAR:
    # Make way for the next message
    undef $msg_txt;
    undef $time;
    undef $revision;
    undef $author;
    undef @branch_roots;

    # Maybe even make way for the next file:
    if ($detected_file_separator) {
      undef $file_full_path;
      undef %branch_names;
      undef %branch_numbers;
    }
  }

  close (LOG_SOURCE);

  ### Process each ChangeLog

  while (my ($dir,$authorhash) = each %grand_poobah)
  {
    &debug ("DOING DIR: $dir\n");

    # Here we twist our hash around, from being
    #   author => time => message => filelist
    # in %$authorhash to
    #   time => author => message => filelist
    # in %changelog.
    #
    # This is also where we merge entries.  The algorithm proceeds
    # through the timeline of the changelog with a sliding window of
    # $Max_Checkin_Duration seconds; within that window, entries that
    # have the same log message are merged.
    #
    # (To save space, we zap %$authorhash after we've copied
    # everything out of it.)

    my %changelog;
    while (my ($author,$timehash) = each %$authorhash)
    {
      my $lasttime;
      my %stamptime;
      foreach my $time (sort {$main::a <=> $main::b} (keys %$timehash))
      {
        my $msghash = $timehash->{$time};
        while (my ($msg,$qunklist) = each %$msghash)
        {
 	  my $stamptime = $stamptime{$msg};
          if ((defined $stamptime)
              and (($time - $stamptime) < $Max_Checkin_Duration)
              and (defined $changelog{$stamptime}{$author}{$msg}))
          {
 	    push(@{$changelog{$stamptime}{$author}{$msg}}, @$qunklist);
          }
          else {
            $changelog{$time}{$author}{$msg} = $qunklist;
            $stamptime{$msg} = $time;
          }
        }
      }
    }
    undef (%$authorhash);

    ### Now we can write out the ChangeLog!

    my ($logfile_here, $logfile_bak, $tmpfile);

    if (! $Output_To_Stdout) {
      $logfile_here =  $dir . $Log_File_Name;
      $logfile_here =~ s/^\.\/\//\//;   # fix any leading ".//" problem
      $tmpfile      = "${logfile_here}.cvs2cl$$.tmp";
      $logfile_bak  = "${logfile_here}.bak";

      open (LOG_OUT, ">$tmpfile") or die "Unable to open \"$tmpfile\"";
    }
    else {
      open (LOG_OUT, ">-") or die "Unable to open stdout for writing";
    }

    print LOG_OUT $ChangeLog_Header;

    if ($XML_Output) {
      print LOG_OUT "<?xml version=\"1.0\"?>\n\n"
          . "<changelog xmlns=\"http://www.red-bean.com/xmlns/cvs2cl/\">\n\n";
    }

    foreach my $time (sort {$main::b <=> $main::a} (keys %changelog))
    {
      my $authorhash = $changelog{$time};
      while (my ($author,$mesghash) = each %$authorhash)
      {
        # If XML, escape in outer loop to avoid compound quoting:
        if ($XML_Output) {
          $author = &xml_escape ($author);
        }

        while (my ($msg,$qunklist) = each %$mesghash)
        {
          my $files               = &pretty_file_list ($qunklist);
          my $header_line;          # date and author
          my $body;                 # see below
          my $wholething;           # $header_line + $body

          # Set up the date/author line.
          # kff todo: do some more XML munging here, on the header
          # part of the entry:
          my ($ignore,$min,$hour,$mday,$mon,$year,$wday)
              = $UTC_Times ? gmtime($time) : localtime($time);

          # XML output includes everything else, we might as well make
          # it always include Day Of Week too, for consistency.
          if ($Show_Day_Of_Week or $XML_Output) {
            $wday = ("Sunday", "Monday", "Tuesday", "Wednesday",
                     "Thursday", "Friday", "Saturday")[$wday];
            $wday = ($XML_Output) ? "<weekday>${wday}</weekday>\n" : " $wday";
          }
          else {
            $wday = "";
          }

          if ($XML_Output) {
            $header_line =
                sprintf ("<date>%4u-%02u-%02u</date>\n"
                         . "${wday}"
                         . "<time>%02u:%02u</time>\n"
                         . "<author>%s</author>\n",
                         $year+1900, $mon+1, $mday, $hour, $min, $author);
          }
          else {
            $header_line =
                sprintf ("%4u-%02u-%02u${wday} %02u:%02u  %s\n\n",
                         $year+1900, $mon+1, $mday, $hour, $min, $author);
          }

          # Reshape the body according to user preferences.
          if ($XML_Output)
          {
            $msg = &preprocess_msg_text ($msg);
            $body = $files . $msg;
          }
          elsif ($No_Wrap)
          {
            $msg = &preprocess_msg_text ($msg);
            $files = wrap ("\t", "	", "$files");
            $msg =~ s/\n(.*)/\n\t$1/g;
            unless ($After_Header eq " ") {
              $msg =~ s/^(.*)/\t$1/g;
            }
            $body = $files . $After_Header . $msg;
          }
          else  # do wrapping, either FSF-style or regular
          {
            if ($FSF_Style)
            {
              $files = wrap ("\t", "        ", "$files");

              my $files_last_line_len = 0;
              if ($After_Header eq " ")
              {
                $files_last_line_len = &last_line_len ($files);
                $files_last_line_len += 1;  # for $After_Header
              }

              $msg = &wrap_log_entry
                  ($msg, "\t", 69 - $files_last_line_len, 69);
              $body = $files . $After_Header . $msg;
            }
            else  # not FSF-style
            {
              $msg = &preprocess_msg_text ($msg);
              $body = $files . $After_Header . $msg;
              $body = wrap ("\t", "        ", "$body");
            }
          }

          $wholething = $header_line . $body;

          if ($XML_Output) {
            $wholething = "<entry>\n${wholething}</entry>\n";
          }

          # One last check: make sure it passes the regexp test, if the
          # user asked for that.  We have to do it here, so that the
          # test can match against information in the header as well
          # as in the text of the log message.

          # How annoying to duplicate so much code just because I
          # can't figure out a way to evaluate scalars on the trailing
          # operator portion of a regular expression.  Grrr.
          if ($Case_Insensitive) {
            unless ($Regexp_Gate && ($wholething !~ /$Regexp_Gate/oi)) {
              print LOG_OUT "${wholething}\n";
            }
          }
          else {
            unless ($Regexp_Gate && ($wholething !~ /$Regexp_Gate/o)) {
              print LOG_OUT "${wholething}\n";
            }
          }
        }
      }
    }

    if ($XML_Output) {
      print LOG_OUT "</changelog>\n";
    }

    close (LOG_OUT);

    if (! $Output_To_Stdout)
    {
      if (-f $logfile_here) {
        rename ($logfile_here, $logfile_bak);
      }
      rename ($tmpfile, $logfile_here);
    }
  }
}


sub parse_date_and_author ()
{
  # Parses the date/time and author out of a line like:
  #
  # date: 1999/02/19 23:29:05;  author: apharris;  state: Exp;

  my $line = shift;

  my ($year, $mon, $mday, $hours, $min, $secs, $author) = $line =~
      m#(\d+)/(\d+)/(\d+)\s+(\d+):(\d+):(\d+);\s+author:\s+([^;]+);#
          or  die "Couldn't parse date ``$line''";
  die "Bad date or Y2K issues" unless ($year > 1969 and $year < 2258);
  # Kinda arbitrary, but useful as a sanity check
  my $time = timegm($secs,$min,$hours,$mday,$mon-1,$year-1900);

  return ($time, $author);
}


# Here we take a bunch of qunks and convert them into printed
# summary that will include all the information the user asked for.
sub pretty_file_list ()
{
  if ($Hide_Filenames and (! $XML_Output)) {
    return "";
  }

  my $qunksref = shift;
  my @qunkrefs = @$qunksref;
  my @filenames;
  my $beauty = "";          # The accumulating header string for this entry.
  my %non_unanimous_tags;   # Tags found in a proper subset of qunks
  my %unanimous_tags;       # Tags found in all qunks
  my %all_branches;         # Branches found in any qunk
  my $common_dir = undef;   # Dir prefix common to all files ("" if none)
  my $fbegun = 0;           # Did we begin printing filenames yet?

  # First, loop over the qunks gathering all the tag/branch names.
  # We'll put them all in non_unanimous_tags, and take out the
  # unanimous ones later.
  foreach my $qunkref (@qunkrefs)
  {
    # Keep track of whether all the files in this commit were in the
    # same directory, and memorize it if so.  We can make the output a
    # little more compact by mentioning the directory only once.
    if ((scalar (@qunkrefs)) > 1)
    {
      if (! (defined ($common_dir)))
      {
        my ($base, $dir);
        ($base, $dir, undef) = fileparse ($$qunkref{'filename'});

        if ((! (defined ($dir)))  # this first case is sheer paranoia
            or ($dir eq "")
            or ($dir eq "./")
            or ($dir eq ".\\"))
        {
          $common_dir = "";
        }
        else
        {
          $common_dir = $dir;
        }
      }
      elsif ($common_dir ne "")
      {
        # Already have a common dir prefix, so how much of it can we preserve?
        $common_dir = &common_path_prefix ($$qunkref{'filename'}, $common_dir);
      }
    }
    else  # only one file in this entry anyway, so common dir not an issue
    {
      $common_dir = "";
    }

    if (defined ($$qunkref{'branch'})) {
      $all_branches{$$qunkref{'branch'}} = 1;
    }
    if (defined ($$qunkref{'tags'})) {
      foreach my $tag (@{$$qunkref{'tags'}}) {
        $non_unanimous_tags{$tag} = 1;
      }
    }
  }

  # Any tag held by all qunks will be printed specially... but only if
  # there are multiple qunks in the first place!
  if ((scalar (@qunkrefs)) > 1) {
    foreach my $tag (keys (%non_unanimous_tags)) {
      my $everyone_has_this_tag = 1;
      foreach my $qunkref (@qunkrefs) {
        if ((! (defined ($$qunkref{'tags'})))
            or (! (grep ($_ eq $tag, @{$$qunkref{'tags'}})))) {
          $everyone_has_this_tag = 0;
        }
      }
      if ($everyone_has_this_tag) {
        $unanimous_tags{$tag} = 1;
        delete $non_unanimous_tags{$tag};
      }
    }
  }

  if ($XML_Output)
  {
    # If outputting XML, then our task is pretty simple, because we
    # don't have to detect common dir, common tags, branch prefixing,
    # etc.  We just output exactly what we have, and don't worry about
    # redundancy or readability.

    foreach my $qunkref (@qunkrefs)
    {
      my $filename    = $$qunkref{'filename'};
      my $revision    = $$qunkref{'revision'};
      my $tags        = $$qunkref{'tags'};
      my $branch      = $$qunkref{'branch'};
      my $branchroots = $$qunkref{'branchroots'};

      $filename = &xml_escape ($filename);   # probably paranoia
      $revision = &xml_escape ($revision);   # definitely paranoia

      $beauty .= "<file>\n";
      $beauty .= "<name>${filename}</name>\n";
      $beauty .= "<revision>${revision}</revision>\n";
      if ($branch) {
        $branch   = &xml_escape ($branch);     # more paranoia
        $beauty .= "<branch>${branch}</branch>\n";
      }
      foreach my $tag (@$tags) {
        $tag = &xml_escape ($tag);  # by now you're used to the paranoia
        $beauty .= "<tag>${tag}</tag>\n";
      }
      foreach my $root (@$branchroots) {
        $root = &xml_escape ($root);  # which is good, because it will continue
        $beauty .= "<branchroot>${root}</branchroot>\n";
      }
      $beauty .= "</file>\n";
    }

    # Theoretically, we could go home now.  But as long as we're here,
    # let's print out the common_dir and utags, as a convenience to
    # the receiver (after all, earlier code calculated that stuff
    # anyway, so we might as well take advantage of it).

    if ((scalar (keys (%unanimous_tags))) > 1) {
      foreach my $utag ((keys (%unanimous_tags))) {
        $utag = &xml_escape ($utag);   # the usual paranoia
        $beauty .= "<utag>${utag}</utag>\n";
      }
    }
    if ($common_dir) {
      $common_dir = &xml_escape ($common_dir);
      $beauty .= "<commondir>${common_dir}</commondir>\n";
    }

    # That's enough for XML, time to go home:
    return $beauty;
  }

  # Else not XML output, so complexly compactify for chordate
  # consumption.  At this point we have enough global information
  # about all the qunks to organize them non-redundantly for output.

  if ($common_dir) {
    # Note that $common_dir still has its trailing slash
    $beauty .= "$common_dir: ";
  }

  if ($Show_Branches)
  {
    # For trailing revision numbers.
    my @brevisions;

    foreach my $branch (keys (%all_branches))
    {
      foreach my $qunkref (@qunkrefs)
      {
        if ((defined ($$qunkref{'branch'}))
            and ($$qunkref{'branch'} eq $branch))
        {
          if ($fbegun) {
            # kff todo: comma-delimited in XML too?  Sure.
            $beauty .= ", ";
          }
          else {
            $fbegun = 1;
          }
          my $fname = substr ($$qunkref{'filename'}, length ($common_dir));
          $beauty .= $fname;
          $$qunkref{'printed'} = 1;  # Just setting a mark bit, basically

          if ($Show_Tags && (defined @{$$qunkref{'tags'}})) {
            my @tags = grep ($non_unanimous_tags{$_}, @{$$qunkref{'tags'}});
            if (@tags) {
              $beauty .= " (tags: ";
              $beauty .= join (', ', @tags);
              $beauty .= ")";
            }
          }

          if ($Show_Revisions) {
            # Collect the revision numbers' last components, but don't
            # print them -- they'll get printed with the branch name
            # later.
            $$qunkref{'revision'} =~ /.+\.([\d])+$/;
            push (@brevisions, $1);

            # todo: we're still collecting branch roots, but we're not
            # showing them anywhere.  If we do show them, it would be
            # nifty to just call them revision "0" on a the branch.
            # Yeah, that's the ticket.
          }
        }
      }
      $beauty .= " ($branch";
      if (@brevisions) {
        if ((scalar (@brevisions)) > 1) {
          $beauty .= ".[";
          $beauty .= (join (',', @brevisions));
          $beauty .= "]";
        }
        else {
          $beauty .= ".$brevisions[0]";
        }
      }
      $beauty .= ")";
    }
  }

  # Okay; any qunks that were done according to branch are taken care
  # of, and marked as printed.  Now print everyone else.

  foreach my $qunkref (@qunkrefs)
  {
    next if (defined ($$qunkref{'printed'}));   # skip if already printed

    if ($fbegun) {
      $beauty .= ", ";
    }
    else {
      $fbegun = 1;
    }
    $beauty .= substr ($$qunkref{'filename'}, length ($common_dir));
    # todo: Shlomo's change was this:
    # $beauty .= substr ($$qunkref{'filename'},
    #              (($common_dir eq "./") ? "" : length ($common_dir)));
    $$qunkref{'printed'} = 1;  # Set a mark bit.

    if ($Show_Revisions || $Show_Tags)
    {
      my $started_addendum = 0;

      if ($Show_Revisions) {
        $started_addendum = 1;
        $beauty .= " (";
        $beauty .= "$$qunkref{'revision'}";
      }
      if ($Show_Tags && (defined $$qunkref{'tags'})) {
        my @tags = grep ($non_unanimous_tags{$_}, @{$$qunkref{'tags'}});
        if ((scalar (@tags)) > 0) {
          if ($started_addendum) {
            $beauty .= ", ";
          }
          else {
            $beauty .= " (tags: ";
          }
          $beauty .= join (', ', @tags);
          $started_addendum = 1;
        }
      }
      if ($started_addendum) {
        $beauty .= ")";
      }
    }
  }

  # Unanimous tags always come last.
  if ($Show_Tags && %unanimous_tags)
  {
    $beauty .= " (utags: ";
    $beauty .= join (', ', keys (%unanimous_tags));
    $beauty .= ")";
  }

  # todo: still have to take care of branch_roots?

  $beauty = "* $beauty:";

  return $beauty;
}


sub common_path_prefix ()
{
  my $path1 = shift;
  my $path2 = shift;

  my ($dir1, $dir2);
  (undef, $dir1, undef) = fileparse ($path1);
  (undef, $dir2, undef) = fileparse ($path2);

  # Transmogrify Windows filenames to look like Unix.
  # (It is far more likely that someone is running cvs2cl.pl under
  # Windows than that they would genuinely have backslashes in their
  # filenames.)
  $dir1 =~ tr#\\#/#;
  $dir2 =~ tr#\\#/#;

  my $accum1 = "";
  my $accum2 = "";
  my $last_common_prefix = "";

  while ($accum1 eq $accum2)
  {
    $last_common_prefix = $accum1;
    last if ($accum1 eq $dir1);
    my ($tmp1) = split (/\//, (substr ($dir1, length ($accum1))));
    my ($tmp2) = split (/\//, (substr ($dir2, length ($accum2))));
    $accum1 .= "$tmp1/" if ((defined ($tmp1)) and $tmp1);
    $accum2 .= "$tmp2/" if ((defined ($tmp2)) and $tmp2);
  }

  return $last_common_prefix;
}


sub preprocess_msg_text ()
{
  my $text = shift;

  # Strip out carriage returns (as they probably result from DOSsy editors).
  $text =~ s/\r\n/\n/g;

  # If it *looks* like two newlines, make it *be* two newlines:
  $text =~ s/\n\s*\n/\n\n/g;

  if ($XML_Output)
  {
    $text = &xml_escape ($text);
    $text = "<msg>${text}</msg>\n";
  }
  elsif (! $No_Wrap)
  {
    # Strip off lone newlines, but only for lines that don't begin with
    # whitespace or a mail-quoting character, since we want to preserve
    # that kind of formatting.  Also don't strip newlines that follow a
    # period; we handle those specially next.  And don't strip
    # newlines that precede an open paren.
    1 while ($text =~ s/(^|\n)([^>\s].*[^.\n])\n([^>\n])/$1$2 $3/g);

    # If a newline follows a period, make sure that when we bring up the
    # bottom sentence, it begins with two spaces.
    1 while ($text =~ s/(^|\n)([^>\s].*)\n([^>\n])/$1$2  $3/g);
  }

  return $text;
}


sub last_line_len ()
{
  my $files_list = shift;
  my @lines = split (/\n/, $files_list);
  my $last_line = pop (@lines);
  return length ($last_line);
}


# A custom wrap function, sensitive to some common constructs used in
# log entries.
sub wrap_log_entry ()
{
  my $text = shift;                # The text to wrap.
  my $left_pad_str = shift;        # String to pad with on the left.

  # These do NOT take left_pad_str into account:
  my $length_remaining = shift;    # Amount left on current line.
  my $max_line_length  = shift;    # Amount left for a blank line.

  my $wrapped_text = "";           # The accumulating wrapped entry.
  my $indentation = "";            # Inherited indentation from prev line.

  my $first_time = 1;              # Is this the first iteration of the loop?

  my @lines = split (/\n/, $text);
  while (@lines)   # Don't use `foreach' here, it won't work.
  {
    my $this_line = shift (@lines);
    chomp $this_line;

    # First, if it matches any of the line-start regexps, then print a
    # newline now...
    if (($this_line =~ /^(\s*)\*\s+[a-zA-Z0-9]/)
        || ($this_line =~ /^(\s*)\* [a-zA-Z0-9_\.\/\+-]+/)
        || ($this_line =~ /^(\s*)\([a-zA-Z0-9_\.\/\+-]+(\)|,\s*)/)
        || ($this_line =~ /^(\s*)- +/)
        || ($this_line =~ /^(\s*)[^\s]+:\s*$/)
        || ($this_line =~ /^(\s*)\*\) +/)
        || ($this_line =~ /^(\s*)[a-zA-Z0-9](\)|\.|\:) +/))
    {
      # Make a line break immediately, unless header separator is set
      # and this line is the first line in the entry, in which case
      # we're getting the blank line for free already and shouldn't
      # add an extra one.
      unless (($After_Header ne " ") and ($first_time)) {
        $wrapped_text .= "\n${left_pad_str}";
      }

      $indentation = $1;
      $length_remaining = $max_line_length - (length ($indentation));
    }

    # Now that any indentation has been preserved, strip off leading
    # whitespace, so up-folding has no ugly side-effects.
    $this_line =~ s/^\s*//;

    # Accumulate the line, and adjust parameters for next line.
    my $this_len = length ($this_line);
    if ($this_len == 0)
    {
      # Blank lines should be preserved, and should cancel any
      # indentation level.
      $this_line = "\n${left_pad_str}";
      $indentation = "";
      $length_remaining = $max_line_length;
    }
    elsif ($this_len >= $length_remaining)
    {
      # Walk backwards from the end.  At first acceptable spot, break
      # a new line.
      my $idx = $length_remaining - 1;
      if ($idx < 0) { $idx = 0 };
      while ($idx > 0)
      {
        if (substr ($this_line, $idx, 1) =~ /\s/)
        {
          my $line_now = substr ($this_line, 0, $idx);
          my $next_line = substr ($this_line, $idx);
          $this_line = $line_now;

          # Clean whitespace off the end.
          chomp $this_line;

          # The current line is ready to be printed.
          $this_line .= "\n${left_pad_str}";

          # Make sure the next line is allowed full room.
          $length_remaining = $max_line_length - (length ($indentation));

          # Strip next_line, but then preserve any indentation.
          $next_line =~ s/^\s*//;
          $next_line = $indentation . $next_line;

          unshift (@lines, $next_line);
          last;
        }
        else
        {
          $idx--;
        }
      }

      # If we just bottomed out, the line is too long, so just break
      # it where the original text does.
      if ($idx == 0)
      {
        if ($length_remaining == ($max_line_length - (length ($indentation))))
        {
          # The line is simply too long -- there is no hope of ever
          # breaking it nicely, so just insert it verbatim, with
          # appropriate padding.
          $this_line = "\n${left_pad_str}${this_line}";
        }
        else
        {
          # Can't break it here, but may be able to on the next round.
          unshift (@lines, $this_line);
          $length_remaining = $max_line_length - (length ($indentation));
          $this_line = "\n${left_pad_str}";
        }
      }
    }
    else  # $this_len < $length_remaining, so tack on what we can.
    {
      # Leave a note for the next iteration.
      $length_remaining = $length_remaining - $this_len;

      if ($this_line =~ /\.$/)
      {
        $this_line .= "  ";
        $length_remaining -= 2;
      }
      else  # not a sentence end
      {
        $this_line .= " ";
        $length_remaining -= 1;
      }
    }

    # Unconditionally indicate that loop has run at least once.
    $first_time = 0;

    $wrapped_text .= "${this_line}";
  }

  # One last bit of padding.
  $wrapped_text .= "\n";

  return $wrapped_text;
}


sub xml_escape ()
{
  my $txt = shift;
  $txt =~ s/&/&amp;/g;
  $txt =~ s/</&lt;/g;
  $txt =~ s/>/&gt;/g;
  return $txt;
}


sub maybe_read_user_map_file ()
{
  my %expansions;

  if ($User_Map_File)
  {
    open (MAPFILE, "<$User_Map_File")
        or die ("Unable to open $User_Map_File ($!)");

    while (<MAPFILE>)
    {
      next if /^\s*#/;  # Skip comment lines.
      next if not /:/;  # Skip lines without colons.

      # It is now safe to split on ':'.
      my ($username, $expansion) = split ':';
      chomp $expansion;
      $expansion =~ s/^'(.*)'$/$1/;
      $expansion =~ s/^"(.*)"$/$1/;

      # If it looks like the expansion has a real name already, then
      # we toss the username we got from CVS log.  Otherwise, keep
      # it to use in combination with the email address.

      if ($expansion =~ /^\s*<{0,1}\S+@.*/) {
        # Also, add angle brackets if none present
        if (! ($expansion =~ /<\S+@\S+>/)) {
          $expansions{$username} = "$username <$expansion>";
        }
        else {
          $expansions{$username} = "$username $expansion";
        }
      }
      else {
        $expansions{$username} = $expansion;
      }
    }

    close (MAPFILE);
  }

  return %expansions;
}


sub parse_options ()
{
  # Check this internally before setting the global variable.
  my $output_file;

  # If this gets set, we encountered unknown options and will exit at
  # the end of this subroutine.
  my $exit_with_admonishment = 0;

  while (my $arg = shift (@ARGV))
  {
    if ($arg =~ /^-h$|^-help$|^--help$|^--usage$|^-?$/) {
      $Print_Usage = 1;
    }
    elsif ($arg =~ /^--debug$/) {        # unadvertised option, heh
      $Debug = 1;
    }
    elsif ($arg =~ /^--version$/) {
      $Print_Version = 1;
    }
    elsif ($arg =~ /^-g$|^--global-opts$/) {
      my $narg = shift (@ARGV) || die "$arg needs argument.\n";
      # Don't assume CVS is called "cvs" on the user's system:
      $Log_Source_Command =~ s/(^\S*)/$1 $narg/;
    }
    elsif ($arg =~ /^-l$|^--log-opts$/) {
      my $narg = shift (@ARGV) || die "$arg needs argument.\n";
      $Log_Source_Command .= " $narg";
    }
    elsif ($arg =~ /^-f$|^--file$/) {
      my $narg = shift (@ARGV) || die "$arg needs argument.\n";
      $output_file = $narg;
    }
    elsif ($arg =~ /^--fsf$/) {
      $FSF_Style = 1;
    }
    elsif ($arg =~ /^-U$|^--usermap$/) {
      my $narg = shift (@ARGV) || die "$arg needs argument.\n";
      $User_Map_File = $narg;
    }
    elsif ($arg =~ /^-W$|^--window$/) {
      my $narg = shift (@ARGV) || die "$arg needs argument.\n";
      $Max_Checkin_Duration = $narg;
    }
    elsif ($arg =~ /^-I$|^--ignore$/) {
      my $narg = shift (@ARGV) || die "$arg needs argument.\n";
      push (@Ignore_Files, $narg);
    }
    elsif ($arg =~ /^-C$|^--case-insensitive$/) {
      $Case_Insensitive = 1;
    }
    elsif ($arg =~ /^-R$|^--regexp$/) {
      my $narg = shift (@ARGV) || die "$arg needs argument.\n";
      $Regexp_Gate = $narg;
    }
    elsif ($arg =~ /^--stdout$/) {
      $Output_To_Stdout = 1;
    }
    elsif ($arg =~ /^--version$/) {
      $Print_Version = 1;
    }
    elsif ($arg =~ /^-d$|^--distributed$/) {
      $Distributed = 1;
    }
    elsif ($arg =~ /^-P$|^--prune$/) {
      $Prune_Empty_Msgs = 1;
    }
    elsif ($arg =~ /^-S$|^--separate-header$/) {
      $After_Header = "\n\n";
    }
    elsif ($arg =~ /^--no-wrap$/) {
      $No_Wrap = 1;
    }
    elsif ($arg =~ /^--gmt$|^--utc$/) {
      $UTC_Times = 1;
    }
    elsif ($arg =~ /^-w$|^--day-of-week$/) {
      $Show_Day_Of_Week = 1;
    }
    elsif ($arg =~ /^-r$|^--revisions$/) {
      $Show_Revisions = 1;
    }
    elsif ($arg =~ /^-t$|^--tags$/) {
      $Show_Tags = 1;
    }
    elsif ($arg =~ /^-b$|^--branches$/) {
      $Show_Branches = 1;
    }
    elsif ($arg =~ /^-F$|^--follow$/) {
      my $narg = shift (@ARGV) || die "$arg needs argument.\n";
      push (@Follow_Branches, $narg);
    }
    elsif ($arg =~ /^--stdin$/) {
      $Input_From_Stdin = 1;
    }
    elsif ($arg =~ /^--header$/) {
      my $narg = shift (@ARGV) || die "$arg needs argument.\n";
      $ChangeLog_Header = &slurp_file ($narg);
      if (! defined ($ChangeLog_Header)) {
        $ChangeLog_Header = "";
      }
    }
    elsif ($arg =~ /^--xml$/) {
      $XML_Output = 1;
    }
    elsif ($arg =~ /^--hide-filenames$/) {
      $Hide_Filenames = 1;
      $After_Header = "";
    }
    else {
      # Just add a filename as argument to the log command
      $Log_Source_Command .= " $arg";
    }
  }

  ## Check for contradictions...

  if ($Output_To_Stdout && $Distributed) {
    print STDERR "cannot pass both --stdout and --distributed\n";
    $exit_with_admonishment = 1;
  }

  if ($Output_To_Stdout && $output_file) {
    print STDERR "cannot pass both --stdout and --file\n";
    $exit_with_admonishment = 1;
  }

  # Or if any other error message has already been printed out, we
  # just leave now:
  if ($exit_with_admonishment) {
    &usage ();
    exit (1);
  }
  elsif ($Print_Usage) {
    &usage ();
    exit (0);
  }
  elsif ($Print_Version) {
    &version ();
    exit (0);
  }

  ## Else no problems, so proceed.

  if ($Output_To_Stdout) {
    undef $Log_File_Name;       # not actually necessary
  }
  elsif ($output_file) {
    $Log_File_Name = $output_file;
  }
}


sub slurp_file ()
{
  my $filename = shift || die ("no filename passed to slurp_file()");
  my $retstr;

  open (SLURPEE, "<${filename}") or die ("unable to open $filename ($!)");
  my $saved_sep = $/;
  undef $/;
  $retstr = <SLURPEE>;
  $/ = $saved_sep;
  close (SLURPEE);
  return $retstr;
}


sub debug ()
{
  if ($Debug) {
    my $msg = shift;
    print STDERR $msg;
  }
}


sub version ()
{
  print "cvs2cl.pl version ${VERSION}; distributed under the GNU GPL.\n";
}


sub usage ()
{
  &version ();
  print <<'END_OF_INFO';
Generate GNU-style ChangeLogs in CVS working copies.

Notes about the output format(s):

   The default output of cvs2cl.pl is designed to be compact, formally
   unambiguous, but still easy for humans to read.  It is largely
   self-explanatory, I hope; the one abbreviation that might not be
   obvious is "utags".  That stands for "universal tags" -- a
   universal tag is one held by all the files in a given change entry.

   If you need output that's easy for a program to parse, use the
   --xml option.  Note that with XML output, just about all available
   information is included with each change entry, whether you asked
   for it or not, on the theory that your parser can ignore anything
   it's not looking for.

Notes about the options and arguments (the actual options are listed
last in this usage message):

  * The -I and -F options may appear multiple times.

  * To follow trunk revisions, use "-F trunk" ("-F TRUNK" also works).
    This is okay because no would ever, ever be crazy enough to name a
    branch "trunk", right?  Right.

  * For the -U option, the UFILE should be formatted like
    CVSROOT/users. That is, each line of UFILE looks like this
       jrandom:jrandom@red-bean.com
    or maybe even like this
       jrandom:'Jesse Q. Random <jrandom@red-bean.com>'
    Don't forget to quote the portion after the colon if necessary.

  * Many people want to filter by date.  To do so, invoke cvs2cl.pl
    like this:
       cvs2cl.pl -l "-d'DATESPEC'"
    where DATESPEC is any date specification valid for "cvs log -d".
    (Note that CVS 1.10.7 and below requires there be no space between
    -d and its argument).

Options/Arguments:

  -h, -help, --help, or -?     Show this usage and exit
  --version                    Show version and exit
  -r, --revisions              Show revision numbers in output
  -b, --branches               Show branch names in revisions when possible
  -t, --tags                   Show tags (symbolic names) in output
  --stdin                      Read from stdin, don't run cvs log
  --stdout                     Output to stdout not to ChangeLog
  -d, --distributed            Put ChangeLogs in subdirs
  -f FILE, --file FILE         Write to FILE instead of "ChangeLog"
  --fsf                        Use this if log data is in FSF ChangeLog style
  -W SECS, --window SECS       Window of time within which log entries unify
  -U UFILE, --usermap UFILE    Expand usernames to email addresses from UFILE
  -R REGEXP, --regexp REGEXP   Include only entries that match REGEXP
  -I REGEXP, --ignore REGEXP   Ignore files whose names match REGEXP
  -C, --case-insensitive       Any regexp matching is done case-insensitively
  -F BRANCH, --follow BRANCH   Show only revisions on or ancestral to BRANCH
  -S, --separate-header        Blank line between each header and log message
  --no-wrap                    Don't auto-wrap log message (recommend -S also)
  --gmt, --utc                 Show times in GMT/UTC instead of local time
  -w, --day-of-week            Show day of week
  --header FILE                Get ChangeLog header from FILE ("-" means stdin)
  --xml                        Output XML instead of ChangeLog format
  --hide-filenames             Don't show filenames (ignored for XML output)
  -P, --prune                  Don't show empty log messages
  -g OPTS, --global-opts OPTS  Invoke like this "cvs OPTS log ..."
  -l OPTS, --log-opts OPTS     Invoke like this "cvs ... log OPTS"
  FILE1 [FILE2 ...]            Show only log information for the named FILE(s)

See http://www.red-bean.com/~kfogel/cvs2cl.shtml for maintenance and bug info.
END_OF_INFO
}

__END__

=head1 NAME

cvs2cl.pl - produces GNU-style ChangeLogs in CVS working copies, by
    running "cvs log" and parsing the output.  Shared log entries are
    unified in an intuitive way.

=head1 DESCRIPTION

This script generates GNU-style ChangeLog files from CVS log
information.  Basic usage: just run it inside a working copy and a
ChangeLog will appear.  It requires repository access (i.e., 'cvs log'
must work).  Run "cvs2cl.pl --help" to see more advanced options.

See http://www.red-bean.com/~kfogel/cvs2cl.shtml for updates, and
for instructions on getting anonymous CVS access to this script.

Maintainer: Karl Fogel <kfogel@red-bean.com>
Please report bugs to <cvs2cl-bugs@red-bean.com>.

=head1 README

This script generates GNU-style ChangeLog files from CVS log
information.  Basic usage: just run it inside a working copy and a
ChangeLog will appear.  It requires repository access (i.e., 'cvs log'
must work).  Run "cvs2cl.pl --help" to see more advanced options.

See http://www.red-bean.com/~kfogel/cvs2cl.shtml for updates, and
for instructions on getting anonymous CVS access to this script.

Maintainer: Karl Fogel <kfogel@red-bean.com>
Please report bugs to <cvs2cl-bugs@red-bean.com>.

=head1 PREREQUISITES

This script requires C<Text::Wrap>, C<Time::Local>, and
C<File::Basename>.
It also seems to require C<Perl 5.004_04> or higher.

=pod OSNAMES

any

=pod SCRIPT CATEGORIES

Version_Control/CVS

=cut


-*- -*- -*- -*- -*- -*- -*- -*- -*- -*- -*- -*- -*- -*- -*- -*- -*- -*-

Note about a bug-slash-opportunity:
-----------------------------------

There's a bug in Text::Wrap, which affects cvs2cl.  This script
reveals it:

  #!/usr/bin/perl -w

  use Text::Wrap;

  my $test_text =
  "This script demonstrates a bug in Text::Wrap.  The very long line
  following this paragraph will be relocated relative to the surrounding
  text:

  ====================================================================

  See?  When the bug happens, we'll get the line of equal signs below
  this paragraph, even though it should be above.";


  # Print out the test text with no wrapping:
  print "$test_text";
  print "\n";
  print "\n";

  # Now print it out wrapped, and see the bug:
  print wrap ("\t", "        ", "$test_text");
  print "\n";
  print "\n";

If the line of equal signs were one shorter, then the bug doesn't
happen.  Interesting.

Anyway, rather than fix this in Text::Wrap, we might as well write a
new wrap() which has the following much-needed features:

* initial indentation, like current Text::Wrap()
* subsequent line indentation, like current Text::Wrap()
* user chooses among: force-break long words, leave them alone, or die()?
* preserve existing indentation: chopped chunks from an indented line
  are indented by same (like this line, not counting the asterisk!)
* optional list of things to preserve on line starts, default ">"

Note that the last two are essentially the same concept, so unify in
implementation and give a good interface to controlling them.

And how about:

Optionally, when encounter a line pre-indented by same as previous
line, then strip the newline and refill, but indent by the same.
Yeah...
