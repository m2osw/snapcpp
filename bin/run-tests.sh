#!/bin/sh -e
#
# Run all the tests

if test -z "${HOME}"
then
    export HOME=`cd && pwd`
fi
if test -z "${USER}"
then
    export USER=`basename ${HOME}`
fi

TOPDIR="`pwd`"
while true
do
    BASE="`basename ${TOPDIR}`"
    if test "${BASE}" = "snapcpp"
    then
        break
    fi
    TOPDIR="`dirname ${TOPDIR}`"
done

cd "${TOPDIR}"

BUILD="${TOPDIR}/BUILD/Debug"
CONTRIBS="`find contrib -maxdepth 1 ! -path 'contrib' -type d | sort`"
SNAPWEBSITES="`find snapwebsites -maxdepth 1 ! -path 'snapwebsites' -type d | sort`"

PUBLISH=false
PUBLISHDIR="/mnt/lcov"
SYNC=false
TYPE=console
while test -n "${1}"
do
    case "${1}" in
    "--console")
        TYPE=console
        shift
        ;;

    "-h"|"--help")
        echo "Usage: `basename ${0}` [--opts]"
        echo "where --opts is one or more of:"
        echo "       --console           output results in plain text"
        echo "  -h | --help              print out this help screen"
        echo "       --html              output results in HTML"
        echo "       --info              display some information (variables)"
        echo "  -p | --publish [dir]     send HTML output to lcov website"
        echo "       --sync              synchronize the git environment"
        exit 1
        ;;

    "--html")
        TYPE=html
        shift
        ;;

    "--info")
        echo "TOPDIR = ${TOPDIR}"
        echo "CONTRIBS = ${CONTRIBS}"
        echo "SNAPWEBSITES = ${SNAPWEBSITES}"
        exit 1
        ;;

    "-p"|"--publish")
        PUBLISH=true
        shift
        if test -n "${1}"
        then
            case "${1}" in
            "-"*)
                # assume this is another option
                ;;

            *)
                PUBLISHDIR="${1}"
                shift
                ;;

            esac
        fi
        ;;

    "--sync")
        SYNC=true
        shift
        ;;

    *)
        echo "error: unknown command line option \"${1}\"."
        exit 1
        ;;

    esac
done

START_DATE=`date -u`

# make sure it's a full path so we an use it anywhere
# WARNING: this test assumes we manually ran a Debug build at least once
CONVERTER="${BUILD}/contrib/snaplogger/tools/convert-ansi"
if test -x "${CONVERTER}"
then
    CONVERT="${CONVERTER} --output-style-tag --no-br"
else
    CONVERT=cat
fi

IS_SNAPWEBSITES_DEFINED=false
if getent passwd snapwebsites >/dev/null \
    && getent group snapwebsites | sed -e 's/.*://' -e 's/,/ /' | grep "\<${USER}\>" >/dev/null \
    && getent group adm | sed -e 's/.*://' -e 's/,/ /' | grep "\<${USER}\>" >/dev/null
then
    IS_SNAPWEBSITES_DEFINED=true
fi

if test "${TYPE}" = "html"
then
    HTMLDIR="${TOPDIR}/BUILD/html"
    rm -rf "${HTMLDIR}"
    mkdir -p "${HTMLDIR}"
    HTML="${HTMLDIR}/index.html"
    echo "<html>" > "${HTML}"
    echo "<head>" >> "${HTML}"
    echo "<title>Test Results</title>" >> "${HTML}"
    echo "<style>" >> "${HTML}"
    echo "body{font-family:sans-serif;}" >> "${HTML}"
    echo "table{border-collapse:collapse;}" >> "${HTML}"
    echo "th{background-color:#f0f0f0;}" >> "${HTML}"
    echo "td,th{border:1px solid black;padding:5px;}" >> "${HTML}"
    echo "tr.no-test{background-color:#eeeeee;}" >> "${HTML}"
    echo "tr.success{background-color:#eeffee;}" >> "${HTML}"
    echo "tr.error{background-color:#ffeeee;}" >> "${HTML}"
    echo "</style>" >> "${HTML}"
    echo "</head>" >> "${HTML}"
    echo "<body>" >> "${HTML}"
    if test "${IS_SNAPWEBSITES_DEFINED}" != "true"
    then
        echo "<p style=\"color:red;font-size:135%\">WARNING: snapwebsites user/group or adm inclusion not properly setup. See bin/install-for-tests.sh for details (${IS_SNAPWEBSITES_DEFINED}).</p>" >> "${HTML}"
    fi
    echo "<table>" >> "${HTML}"
    echo "<tr><th>Project</th><th>Test Results</th><th>Documentation</th></tr>" >> "${HTML}"
fi

if test "${SYNC}" = "true"
then
    # sync the repo
    #
    if test "${TYPE}" = "html"
    then
        SYNC_OUTPUT="${HTMLDIR}/sync.html"
        echo "<html>" > "${SYNC_OUTPUT}"
        echo "<head>" >> "${SYNC_OUTPUT}"
        echo "<title>Source Synchronization</title>" >> "${SYNC_OUTPUT}"
        echo "</head>" >> "${SYNC_OUTPUT}"
        echo "</head>" >> "${SYNC_OUTPUT}"
        echo "<body>" >> "${SYNC_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${SYNC_OUTPUT}"
        echo "<h1>Source Synchronization</h1>" >> "${SYNC_OUTPUT}"
        echo "<p>`date -u`</p>" >> "${SYNC_OUTPUT}"
        echo "<pre>" >> "${SYNC_OUTPUT}"
        git pull 2>&1 | ${CONVERT} >> "${SYNC_OUTPUT}"
        bin/check-status.sh --latest 2>&1 | ${CONVERT} >> "${SYNC_OUTPUT}"
        echo "</pre>" >> "${SYNC_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${SYNC_OUTPUT}"
        echo "</body>" >> "${SYNC_OUTPUT}"
        echo "</html>" >> "${SYNC_OUTPUT}"
    else
        git pull
        bin/check-status.sh --latest
    fi

    # We want the docs to be rebuilt so delete the existing version first
    # (i.e. the ./mk --documentation is not sufficient because it would not get installed)
    #
    rm -rf ${BUILD}/contrib/*/doc/*-doc-*

    # and after sync-ing we need to recompile
    #
    if test "${TYPE}" = "html"
    then
        COMPILE_OUTPUT="${HTMLDIR}/compile.html"
        echo "<html>" > "${COMPILE_OUTPUT}"
        echo "<head>" >> "${COMPILE_OUTPUT}"
        echo "<title>Source Synchronization</title>" >> "${COMPILE_OUTPUT}"
        echo "</head>" >> "${COMPILE_OUTPUT}"
        echo "</head>" >> "${COMPILE_OUTPUT}"
        echo "<body>" >> "${COMPILE_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${COMPILE_OUTPUT}"
        echo "<h1>Snap! C++ Compile</h1>" >> "${COMPILE_OUTPUT}"
        echo "<pre>" >> "${COMPILE_OUTPUT}"

        # We want to detect whether make returns an error or not
        # so we save the output in an intermediate file
        #
        RAW_COMPILE_OUTPUT="${HTMLDIR}/raw-compile.txt"
        if ! make -C BUILD/Debug > "${RAW_COMPILE_OUTPUT}" 2>&1
        then
            echo "<p style=\"color:red;font-size:135%\">ERROR: the <a href=\"compile.html\" rel=\"nofollow\">compile step</a> failed.</p>" >> "${HTML}"
        fi
        ${CONVERT} "${RAW_COMPILE_OUTPUT}" >> "${COMPILE_OUTPUT}" 2>&1
        rm -f "${RAW_COMPILE_OUTPUT}"

        echo "</pre>" >> "${COMPILE_OUTPUT}"
        echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${COMPILE_OUTPUT}"
        echo "</body>" >> "${COMPILE_OUTPUT}"
        echo "</html>" >> "${COMPILE_OUTPUT}"
    else
        make -C BUILD/Debug
    fi
fi

FAILURES=0
#  ${SNAPWEBSITES} -- skip on snapwebsites/... tests for now since those are not up to date yet
for d in ${CONTRIBS}
do
    cd "${d}"
    PROJECT_NAME=`basename ${d}`
    if test "${PROJECT_NAME}" = "snapcatch2"
    then
        # The docs for Catch2 don't use the project name
        # (also below we convert the .md files to .html)
        PROJECT_PATH="Catch2"
    else
        PROJECT_PATH="${PROJECT_NAME}"
    fi
    DOCUMENTATION="<td><a href=\"../docs/${PROJECT_PATH}\">${PROJECT_NAME}</a>"
    if test "${PROJECT_NAME}" = "eventdispatcher"
    then
        # eventdispatcher includes an extra two sub-projects with their own docs
        DOCUMENTATION="${DOCUMENTATION}<br/><a href=\"../docs/cppprocess\">cppprocess</a>"
        DOCUMENTATION="${DOCUMENTATION}<br/><a href=\"../docs/snaplogger-network\">snaplogger-network</a>"
    fi
    DOCUMENTATION="${DOCUMENTATION}</td>"
    if test -d tests
    then
        if test "${TYPE}" = "console"
        then
            echo "--- RUNNING TESTS OF: ${d} ---"
        fi
        OUTPUTDIR="${BUILD}/${d}"
        mkdir -p "${OUTPUTDIR}"
        OUTPUT="${OUTPUTDIR}/test-full-output.txt"

        if ! ./mk --test >"${OUTPUT}" 2>&1
        then
            if test "${TYPE}" = "html"
            then
                ERROR="${PROJECT_NAME}.html"
                echo "<tr class=\"error\"><td>${d}</td><td>error -- <a href=\"${ERROR}\" rel=\"nofollow\">see output</a></td>${DOCUMENTATION}</tr>" >> "${HTML}"

                echo "<html>" > "${HTMLDIR}/${ERROR}"
                echo "<head>" >> "${HTMLDIR}/${ERROR}"
                echo "<title>Output of ${d} tests</title>" >> "${HTMLDIR}/${ERROR}"
                echo "<meta name=\"robots\" content=\"noindex\"/>" >> "${HTMLDIR}/${ERROR}"
                echo "</head>" >> "${HTMLDIR}/${ERROR}"
                echo "<body>" >> "${HTMLDIR}/${ERROR}"
                echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${HTMLDIR}/${ERROR}"
                echo "<pre>" >> "${HTMLDIR}/${ERROR}"
                ${CONVERT} "${OUTPUT}" >> "${HTMLDIR}/${ERROR}"
                echo "</pre>" >> "${HTMLDIR}/${ERROR}"
                echo "<p><a href=\"index.html\">Back to list</a></p>" >> "${HTMLDIR}/${ERROR}"
                echo "</body>" >> "${HTMLDIR}/${ERROR}"
                echo "</html>" >> "${HTMLDIR}/${ERROR}"
            else
                echo "   an error occurred, see output here: ${OUTPUT}"
                echo
            fi
            FAILURES=`expr ${FAILURES} + 1`
        else
            if test "${TYPE}" = "html"
            then
                echo "<tr class=\"success\"><td>${d}</td><td>success</td>${DOCUMENTATION}</tr>" >> "${HTML}"
            fi
        fi
    else
        if test "${TYPE}" = "html"
        then
            echo "<tr class=\"no-test\"><td>${d}</td><td>no tests found -- skipped</td>${DOCUMENTATION}</tr>" >> "${HTML}"
        else
            echo "--- SKIPPING: ${d} ---"
        fi
    fi
    cd "${TOPDIR}"
done

END_DATE=`date -u`

if test "${TYPE}" = "html"
then
    echo "</table>" >> "${HTML}"
    if test "${SYNC}" = "true"
    then
        echo "<p><a href=\"sync.html\">Sync output</a></p>" >> "${HTML}"
        echo "<p><a href=\"compile.html\">Snap! C++ Compile</a></p>" >> "${HTML}"
    fi
    if test ${FAILURES} -ne 0
    then
        PLURAL="s"
        if test ${FAILURES} -eq 1
        then
            PLURAL=""
        fi
        echo "<p>Got ${FAILURES} error${PLURAL}.</p>" >> "${HTML}"
    fi
    START_SECONDS=`date -d "${START_DATE}" +%s`
    END_SECONDS=`date -d "${END_DATE}" +%s`
    set +e
    DURATION=`expr ${END_SECONDS} - ${START_SECONDS}`
    DURATION_HOURS=`expr ${DURATION} / 3600`
    DURATION_MINUTES=`expr \( ${DURATION} % 3600 \) / 60`
    DURATION_SECONDS=`expr ${DURATION} % 60`
    set -e
    DURATION_HHMMSS=`printf "%02d:%02d:%02d" ${DURATION_HOURS} ${DURATION_MINUTES} ${DURATION_SECONDS}`
    echo "<p>Process started on ${START_DATE} and ending on ${END_DATE} (elapsed time: ${DURATION_HHMMSS}).</p>" >> "${HTML}"
    echo "<br/>" >> "${HTML}"
    echo '<div style="text-align:center;"><a href="https://snapwebsites.org/">Snap C++</a> | <a href="..">List of projects</a></div>' >> "${HTML}"
    echo "</body>" >> "${HTML}"
    echo "</html>" >> "${HTML}"

    # publish the results, that means deleting now working tests
    # and then replacing with newly failing ones (or none if it's
    # all in working order which is probably going to be rare)
    #
    if test "${PUBLISH}" = "true" -a -d "${PUBLISHDIR}"
    then
        if test -d "${PUBLISHDIR}/tests"
        then
            rm -rf "${PUBLISHDIR}/tests/"*
            cp -r "${HTMLDIR}/"* "${PUBLISHDIR}/tests/."
        fi
        if test -d "${PUBLISHDIR}/docs"
        then
            rm -rf "${PUBLISHDIR}/docs/"*
            for d in "${BUILD}/dist/share/doc/"*
            do
                PROJECT=`basename "${d}"`
                mkdir -p "${PUBLISHDIR}/docs/${PROJECT}"
                if test -d "${d}/html"
                then
                    # doxygen adds that /html/... folder
                    #
                    cp -r "${d}/html/"* "${PUBLISHDIR}/docs/${PROJECT}/."
                else
                    # Catch2 does not use doxygen
                    #
                    cp -r "${d}/"* "${PUBLISHDIR}/docs/${PROJECT}/."
                fi
            done

            HTML_INDEX="${BUILD}/dist/share/doc/Catch2/index.html"
            echo "<html>" > ${HTML_INDEX}
            echo "<head>" >> ${HTML_INDEX}
            echo "<title>Catch2 Documentation</title>" >> ${HTML_INDEX}
            echo "</head>" >> ${HTML_INDEX}
            echo "<body>" >> ${HTML_INDEX}
            echo "<ul>" >> ${HTML_INDEX}
            for m in "${BUILD}/dist/share/doc/Catch2/"*.md
            do
                HTML_FILENAME="`echo "${m}" | sed -e 's/\.md$//'`.html"
                # the links use "<name>.md" so we have to change them to use .html
                pandoc --from=markdown --to=html --standalone "${m}" \
                    | sed -e 's/href="\([^#][^"]\+\)\.md\(#[^"]\+\)\?"/href="\1.html\2"/' "${HTML_FILENAME}"
                BASENAME=`basename ${m} .md`
                echo "<li><a href=\"${BASENAME}.html\">${BASENAME}</a></li>" >> ${HTML_INDEX}
            done
            echo "</ul>" >> ${HTML_INDEX}
            echo "</body>" >> ${HTML_INDEX}
            echo "</html>" >> ${HTML_INDEX}
        fi
    fi
fi

if test ${FAILURES} -eq 0
then
    exit 0
fi

echo "error: ${FAILURES} projects failed."

# vim: ts=4 sw=4 et
