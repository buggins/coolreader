#!/usr/bin/perl
use strict;
use warnings;

my @classes = qw(
  org/coolreader/crengine/WordTimingAudiobookMatcher.java
  org/coolreader/crengine/SentenceInfo.java
  org/coolreader/crengine/L.java
  org/coolreader/crengine/Logger.java
  org/coolreader/crengine/SentenceInfoCache.java
);

sub main(@){
  system "rm", "-rf", "run-main-tmp/";
  system "mkdir", "run-main-tmp/";
  my @restoreCommands;
  for my $debugClassFile(glob "debug/*"){
    if($debugClassFile =~ /^(?:.*\/)?([^\/]+)\.(\w+)\.java$/){
      my ($pkg, $class) = ($1, $2);
      my $targetDir = $pkg;
      $targetDir =~ s/\./\//g;
      $targetDir = "src/$targetDir";
      system "cp", "$targetDir/$class.java", "run-main-tmp/$pkg\.$class.java";
      push @restoreCommands, ["mv", "run-main-tmp/$pkg\.$class.java", "$targetDir/$class.java"];
      system "cp", $debugClassFile, "$targetDir/$class.java";
    }else{
      die "ERROR: malformed debug class $debugClassFile\n";
    }
  }
  system "javac", map {"src/$_"} @classes;
  if($? != 0){
    die "java failed\n";
  }
  system "java",
    "-cp", "src",
    "org.coolreader.crengine.WordTimingAudiobookMatcher",
    @_;

  for my $cmd(@restoreCommands){
    system @$cmd;
  }
  system "rm", "-rf", "run-main-tmp/";
  system "find", "src/", "-name", "*.class", "-delete";
}

&main(@ARGV);
