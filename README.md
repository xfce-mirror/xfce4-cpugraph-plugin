# xfce4-cpugraph-plugin

CPU Graph plugin for the Xfce4 panel. This plugin displays a graph of the current CPU load and its history over time. Supports Linux, and it is partially supported on FreeBSD, NetBSD, OpenBSD, GNU/kFreeBSD and Solaris. Fully translated into more than 20 languages, with another 30 partially supported languages.

----

### Homepage

[CPU Graph documentation](https://docs.xfce.org/panel-plugins/xfce4-cpugraph-plugin/start)

### Changelog

See [NEWS](https://gitlab.xfce.org/panel-plugins/xfce4-cpugraph-plugin/-/blob/master/NEWS) for details on changes and fixes made in the current release.

### Source Code Repository

[CPU graph source code](https://gitlab.xfce.org/panel-plugins/xfce4-cpugraph-plugin)

### Download a Release Tarball

[CPU Graph archive](https://archive.xfce.org/src/panel-plugins/xfce4-cpugraph-plugin/)
    or
[CPU Graph tags](https://gitlab.xfce.org/panel-plugins/xfce4-cpugraph-plugin/-/tags)

### Installation

From source:

    % cd xfce4-cpugraph-plugin
    % ./autogen.sh
    % make
    % make install

From release tarball:

    % tar xf xfce4-cpugraph-plugin-<version>.tar.bz2
    % cd xfce4-cpugraph-plugin-<version>
    % ./configure
    % make
    % make install


### Reporting Bugs

Visit the [reporting bugs](https://docs.xfce.org/panel-plugins/xfce4-cpugraph-plugin/bugs) page to view currently open bug reports and instructions on reporting new bugs or submitting bugfixes.

---

### Usage

Just add it to your Xfce panel, and it should display the CPU usage in an easy to understand way.

The plugin can display the current CPU usage, CPU usage history, or both.

A left click on the plugin will launch xfce4-taskmanager, or if you don't have it, htop or top in a terminal.

A right click gives access to the usual Xfce plugin context menu, from which the properties editor can be opened. The properties are split into multiple tabs.

The first tab allows changing the appearance of the CPU history graph. You can select between 4 display modes and 3 color modes, and pick the various colors used.

The properties dialog is designed so that changing a property automatically leaves enabled only those dialog fields which are applicable to the current situation.

Short descriptions of some of the available options:

* **Update interval:** How often the graph is to be refreshed with CPU usage data.

* **Tracked core:** On multi core or multi CPU systems, allows to pick whether CPU Graph should display data representing all cores/CPUs or only a specific one.

* **Width / Height:** Depending on the orientation of the Xfce panel, allows to change the width or height of the CPU Graph plugin.

* **Threshold (%):** Do not display CPU usage data-points which are lower than this percentage.

* **Non-linear time-scale:** Enables the plugin to display more data-points of history than with a linear time-scale, with up to 16 hours of CPU usage history packed into a width of just 128 pixels.

* **Per-core history graphs:** Selects between one history graph displaying average CPU usage, or multiple graphs showing CPU usages individually.

* **Show current usage bars:** When checked, in addition to the history graph, the plugin shows one bar per core/CPU, displaying the current load of that core/CPU.

* **Associated command:** Sets the command to run when the plugin is left-clicked.

* **Run in a terminal:** When checked, the associated command will be run from a terminal, rather than as a graphical application. Useful for example when the associated command is "htop" or "top".

* **Highlight sub-optimal SMT scheduling:** Highlights potentially sub-optimal placement of threads on CPUs with simultaneous multi-threading (hyper-threading).