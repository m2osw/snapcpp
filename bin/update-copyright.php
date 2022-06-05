<?php
//  Copyright (c) 2018  Made to Order Software Corp.
// 
//  We use this script to go through our source files and bump the copyright
//  notice to the current year.
// 
//  The script expects a specific line with our copyright notice, although it
//  uses a couple of regex to allow for a few versions. If the date does not
//  match the current year, the file gets updated.
// 
//  You should make sure that everything is checked in before you run the
//  script and then verify with `git diff` to make sure it only updated the
//  proper lines of code.

$errcount = 0;
$current_year = date('Y');
$count_directories = 0;
$count_total_files = 0;
$count_modified_files = 0;
$count_binary_files = 0;

function update_copyright_notice($filename)
{
    global $errcount, $current_year;

    // read the data
    //
    $data = file_get_contents($filename);
    if($data === false)
    {
        echo "error: could not read file \"$filename\"...";
        ++$errcount;
        return false;
    }

    // got the file contents, search for a copyright notice
    //
    // We have a few types of notices:
    //
    //    (a)  Copyright (c) 2011-2018 by Made to Order Software Corporation -- All Rights Reserved.
    //
    //    (b)  Copyright (c) 2011-2018 Made to Order Software Corp.
    //
    //    (c)  Copyright 2013-2018 (c) Made to Order Software Corporation  All rights reverved.
    //
    // (b) and (c) are the old format, all get changed to format (a).
    // The copyright "symbol" (i.e. the "(c)") may be in uppercase, we
    // catch those too as our searches are case insensitive
    //
    $count = preg_match_all("/copyright (.*) (?:by )?made to order software corp(?:\.)?[^\\\"'.\n\r]*/i"
                          , $data
                          , $matches
                          , PREG_PATTERN_ORDER | PREG_OFFSET_CAPTURE);

    if($count === false
    || $count == 0)
    {
        // no matches, return as is
        //
        return false;
    }
//echo "------------------- Matches = ", $filename, "\n";
//var_dump($matches);
//echo "\n";
//return false;

    // we got a match now we have an array of arrays of arrays with:
    //
    //   [0][0..n] -- full patterns match
    //   [1][0..n] -- the "(c) + date" data
    //

    // WARNING: Process in reverse so that way the offsets stay valid.
    //          If we were to change the first notice first,
    //          we would likely lose the offset of the following notices!
    //          By changing the last notice first, we do not change
    //          the offset for the previous notices.
    //
    $converted = 0;
    $i = $count;
    while($i > 0)
    {
        --$i;

        $c_date = $matches[1][$i][0];
        if(preg_match("/\[year\]/", $c_date) > 0
        || preg_match("/UTC_BUILD_YEAR/", $c_date) > 0)
        {
            // ignore the "Copyright [year] (c) by ..." notices in our XML
            // files which are used on the client's side with the current
            // year
            //
            // and also those tools that use UTC_BUILD_YEAR to include the
            // current year in a tool... (with `BOOST_PP_STRINGIZE()`)
            //
            continue;
        }

        $n = preg_match("/([0-9]{4})(?:\-([0-9]{4}))?/", $c_date, $m_date);
        if($n != 1)
        {
            echo "error:$filename: could not properly determine dates in \"$c_date\".";
            exit(1);
        }

        $n_date = "";
        if(count($m_date) == 2)
        {
            // no range, just one date, check if equal to current date
            // if not, create a new range
            //
            if($m_date[1] != $current_year)
            {
                $n_date = $m_date[1] . "-" . $current_year;
            }
        }
        else
        {
            // range does not yet end with current year?
            //
            if($m_date[2] != $current_year)
            {
                $n_date = $m_date[1] . "-" . $current_year;
            }
        }
        
        // the dates are correct, keep notice as is
        //
        if(strlen($n_date) == 0)
        {
            continue;
        }

        //echo "----------------------------------- Dates For ", $filename, "\n";
        //var_dump($m_date);
        //echo "----------------------------------- For ", $filename, "\n";
        //echo "+++ convert: [", $matches[0][$i][0], "]\n";
        //echo "+++      to: [Copyright (c) ", $n_date, "  Made to Order Software Corp.  All Rights Reserved]\n\n";

        //$matches[0][$i][0] string to replace
        //$matches[0][$i][1] start offset

        $new_notice = "Copyright (c) " . $n_date . "  Made to Order Software Corp.  All Rights Reserved";

        $data = substr($data, 0, $matches[0][$i][1])
              . $new_notice
              . substr($data, $matches[0][$i][1] + strlen($matches[0][$i][0]));

        ++$converted;
    }

    if($converted > 0)
    {
        //echo "----------------------------------- Converted ", $converted, " in ", $filename, "\n";
        //echo $data;
        file_put_contents($filename, $data);

        return true;
    }

    return false;
}


function process($path)
{
    global $errcount, $count_modified_files, $count_binary_files, $count_total_files, $count_directories;

    ++$count_directories;

    $dir = dir($path);

    for(;;)
    {
        // get next filename
        //
        $entry = $dir->read();
        if(empty($entry))
        {
            break;
        }

        // ignore all hidden files
        //
        if($entry[0] == ".")
        {
            continue;
        }

        // ignore files that we know are binary files
        //
        $n = preg_match("/\.(png|jpg|jpeg|gif|eot|svg|otf|eot|ttf|woff2?|zip|ico|eps|xcf|odg|ods|odt|odf|pdf|pem|crt|key|ui|desktop)$/i", $entry);
        if($n != false
        && $n > 0)
        {
            // skip binary file
            //
//echo "skip $entry (binary file!)\n";
            ++$count_binary_files;
            continue;
        }

        // ignore directories we know are to going to include source data
        //
        if($entry == "tmp"
        || $entry == "BUILD")
        {
            continue;
        }

        // generate the "full" path from where we are
        //
        $subpath = $path . "/" . $entry;

        // if we found a directory, process it
        //
        if(is_dir($subpath))
        {
            process($subpath);
        }
        else
        {
            ++$count_total_files;

            if(update_copyright_notice($subpath))
            {
                ++$count_modified_files;
            }
        }
    }
}

process(".");


echo "\n";
echo "Worked on current year: ", $current_year, "\n";
echo "Processed $count_directories directories.\n";
echo "Skip $count_binary_files binary files.\n";
echo "Processed $count_modified_files source files out of $count_total_files.\n";

// if correct we can remove this warning
echo "\n";
echo "+++ WARNING: was snapwebsites/snapcgi/conf/maintenance.html updated properly? The copyright date is in two places. +++\n";
echo "\n";

if($errcount > 0)
{
    echo "\n",
         "WARNING: $errcount occurred while processing your data.\n";
}

// vim: ts=4 sw=4 et
