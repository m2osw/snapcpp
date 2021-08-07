
# Packages

The packages on Launchpad are built and saved under:

http://ppa.launchpad.net/snapcpp/ppa/ubuntu/

This is a standard Debian repository. So you can first download the Packages.xz
and then look for the file(s) you're interested in and use the URLs in there
to download the actualy .deb files to install.

# Launchpad API

The projects on Launchpad are accessible via the
[Launchpad API](https://launchpad.net/+apidoc/devel.html). By default
the API returns JSON data which we can readily use to determine whether a
version is available and/or a build is finished.

Here is the URL one can use to get the build records of the libtld library:

    https://api.launchpad.net/devel/~snapcpp/+archive/ubuntu/ppa?ws.op=getBuildRecords&source_name=libtld

This gives us a complete list of all the builds that ever happened. Below is
one that was successful and happened on Jun 6, 2021 for amd64 / hirsute.
Especially, there is a "buildstate" field which tells us the current state
of the build process for this source:

    "buildstate": "Successfully built",
    "buildstate": "Failed to build",

All the possible states are listed on the API documentation page:

* Needs building
* Successfully built
* Failed to build
* Dependency wait
* Chroot problem
* Build for superseded Source
* Currently building
* Failed to upload
* Uploading build
* Cancelling build
* Cancelled build

One complete entry:

    {
        "self_link": "https://api.launchpad.net/devel/~snapcpp/+archive/ubuntu/ppa/+build/21663540",
        "web_link": "https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+build/21663540",
        "resource_type_link": "https://api.launchpad.net/devel/#build",
        "datecreated": "2021-06-05T00:46:14.957625+00:00",
        "date_started": "2021-06-05T00:46:33.709381+00:00",
        "datebuilt": "2021-06-05T00:50:18.253535+00:00",
        "duration": "0:03:44.544154",
        "date_first_dispatched": "2021-06-05T00:46:33.709381+00:00",
        "builder_link": "https://api.launchpad.net/devel/builders/lcy01-amd64-027",
        "buildstate": "Successfully built",
        "build_log_url": "https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+build/21663540/+files/buildlog_ubuntu-hirsute-amd64.libtld_1.6.2.1~hirsute_BUILDING.txt.gz",
        "title": "amd64 build of libtld 1.6.2.1~hirsute in ubuntu hirsute RELEASE",
        "dependencies": null,
        "archive_link": "https://api.launchpad.net/devel/~snapcpp/+archive/ubuntu/ppa",
        "pocket": "Release",
        "upload_log_url": null,
        "distribution_link": "https://api.launchpad.net/devel/ubuntu",
        "current_source_publication_link": "https://api.launchpad.net/devel/~snapcpp/+archive/ubuntu/ppa/+sourcepub/12478027",
        "source_package_name": "libtld",
        "source_package_version": "1.6.2.1~hirsute",
        "arch_tag": "amd64",
        "can_be_rescored": false,
        "can_be_retried": false,
        "can_be_cancelled": false,
        "changesfile_url": "https://launchpad.net/~snapcpp/+archive/ubuntu/ppa/+build/21663540/+files/libtld_1.6.2.1~hirsute_amd64.changes",
        "score": null,
        "external_dependencies": null,
        "http_etag": "\"8764d5102f4fba14a679c5f4467fc4d122686e71-eaa19a9ed5d396a90c5f0a3265d6c2d0fb3ceeaf\""
    },


# Asking Questions on Launchpad

Here is a URL that a staff member from Launchpad gave me on Stackoverflow.com:

https://answers.launchpad.net/launchpad

He says this is the best place to ask Launchpad related questions.

On my end, I first asked on the Ask Ubuntu stack:

https://askubuntu.com/questions/1338731/how-do-i-programmatically-detect-that-my-launchpad-build-process-is-done

Then someone told me to ask on Launchpad although I first decided to post
a bug report there, here are the links:

* Bug

    https://bugs.launchpad.net/launchpadlib/+bug/1928872

* Question

    https://answers.launchpad.net/ubuntu/+source/python-launchpadlib/+question/697171

All in all, there is a fairly simple URL, but sadly it's not clearly
documented. Much info on how to manage bugs, but not builds.




# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snapcpp/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
