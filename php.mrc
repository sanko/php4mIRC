; PHP4mIRC support script for version 1.0
;
; Written by Sanko Robinson <sanko@cpan.org>
;
; This file is not needed to use perl4mirc.dll but
; provides a simplified interface to access it.
;
; See README.txt for information on how to use
; the commands defined here, or look at the
; Examples below.
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of The Artistic License 2.0.  See the F<LICENSE>
; file included with this distribution or
; http://www.perlfoundation.org/artistic_license_2_0.  For
; clarification, see http://www.perlfoundation.org/artistic_2_0_notes.

; Convenience methods
alias php_dll { return " $+ $scriptdirphp4mIRC.dll $+ "  }
alias php_unload { dll -u $php_dll }
alias php_str { return $qt($replace($1,\,/,$,\$)) }
alias php {  if ($isid)  return $dll($php_dll,php_eval_string,$1-) | dll $php_dll php_eval_string $1-; }

; PHP interpeter bridge methods for embedded scripts
alias php_embed { php mIRC::eval_embed( $php_str($1) $+ , $2-) | return $false }
alias use_php { return $!php_embed($script,$scriptline) }

; Initialization callback
on *:SIGNAL:PHP_ONLOAD: {
  ;php mIRC->var('PHPVer') = phpversion();
  ;php mIRC->var('version') = qq[$mIRC::VERSION]
  echo $color(info2) -ae * Loaded PHP4mIRC %version (using PHP %PHPVer $+ ). Edit line $scriptline of $qt($remove($script,$mircdir)) to change or remove this message.
  ;php mIRC->unset('PHPVer')
  ;php mIRC->unset('version')
}

on *:SIGNAL:PHP_UNLOAD: {
  echo $color(info2) -ae * Unloaded PHP4mIRC
}

; Standard input/output handling
on *:SIGNAL:PHP_STDOUT:if ($1 != $null) echo -a $1-
on *:SIGNAL:PHP_STDERR:if ($1 != $null) echo $color(info) -a $1-

on *:LOAD: { echo $color(info2) -ae * Running /php_test to see if PHP works: | php_test }

; Examples

; One-liners

; Classic hello world
alias php_hello_world { php echo "Hello, world!"; }

; Version
alias php_version { if ($isid) return $dll($php_dll,version,$1-) | dll $php_dll version $1- }

; PHP timer-delays (needs multithreaded PHP)
; Use threads only at your own risk!
alias php_threads { 
  ;php use threads; async{sleep 10; print 'threads test complete!'}; print 'threads test... start!'; 
}

; Shows how to pass data to and from PHP when certain identifiers
; are not accessible such as $1-
alias perl_strlen {
  set %data $1-
  ;php echo 'len:' . length(mIRC->var('data'));
  unset %data
}

; Embedded PHP

; Test method
alias php_test {
  if $($use_php,2) {
    mIRC->linesep("-a");
    my @array = qw[3 5 1 2 4 9 7 6];
    print 'Testing PHP';
    print '  Original array: ' . join( ', ', @array );
    print '  Sorted array  : ' . join( ', ', sort @array );
  }
}

; Lists the modules currently loaded in PHP
alias PHP_list_modules {
  if $($use_php,2) {
    my @modules;
    for my $module(keys %INC) {
      if ($module =~ m[\.pm$]) {
        $module =~ s|/|::|g;
        $module =~ s|.pm$||;
      }
      push @modules, $module;
    }
    # Bring information back to mIRC in a var rather
    # than using the mirc proc to /echo the results
    mIRC->var('modules') = join(q[, ], sort {lc $a cmp lc $b} @modules);
  }
  echo -a PHP Modules: %modules
  unset %modules
}

; REAL inline C :D
; You asked for it, so here it is...
; Requires you to install this script without spaces in the path
alias inlinec {
  if $($use_php,2) {
    use Inline (C => <<'');
    int add(int x, int y)      { return x + y; }
    int subtract(int x, int y) { return x - y; }

    print "9 + 16 = " . add(9, 16) . "\n";
    print "9 - 16 = " . subtract(9, 16) . "\n";
  }
}
