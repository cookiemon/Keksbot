#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sqlite3
import pandas as pd
import numpy as np
import os
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

max_active = 10

plt.xkcd()
mpl.rcParams['figure.figsize'] = (8.0, 4.0)

dbfile = "~/Downloads/keksbot.db"
outdir = "~/Downloads"
dbfile = os.path.expanduser(dbfile)
outdir = os.path.expanduser(outdir)

query = "SELECT timestamp, COALESCE(aliases.nick, stats.nick) as nick, charcount, wordcount, linecount " \
 + "FROM stats LEFT JOIN aliases ON stats.nick = aliases.alias AND stats.server = aliases.server " \
 + "WHERE stats.server = ?1 AND stats.channel = ?2;"

with sqlite3.connect(dbfile) as c:
    big_table = pd.read_sql_query(query, c, params=("freenode", "#kitinfo"))
big_table['date'] = pd.to_datetime(big_table['timestamp'], unit='s')

sums = big_table.groupby('nick').agg(np.sum).sort_values('charcount', ascending=False)
most_active = list(sums.index[:max_active])

table = big_table[big_table['nick'].isin(most_active)]

daily = table.groupby(['nick', pd.Grouper(freq='D', key='date')]).sum()
monthly = table.groupby(['nick', pd.Grouper(freq='M', key='date')]).sum()
aggregate_all = table.groupby('nick').sum()
aggregate_all['color'] = pd.Series(['C' + str(most_active.index(i)) for i in aggregate_all.index], index=aggregate_all.index)
aggregate_all = aggregate_all.sort_values('color')
# There was a serious feature before september 2016,
# so use data after that for aggregation purposes
table_late = table[table['date'].dt.year >= 2017]
daily_late = table_late.groupby(['nick', pd.Grouper(freq='D', key='date')]).sum()
monthly_late = table_late.groupby(['nick', pd.Grouper(freq='M', key='date')]).sum()
aggregate = table_late.groupby('nick').sum()
aggregate['color'] = pd.Series(['C' + str(most_active.index(i)) for i in aggregate.index], index=aggregate.index)
aggregate = aggregate.sort_values('color')
barcolors = list(aggregate['color'])

def doplot(data, plotfun, brati=True):
    for i in most_active:
        if not brati and i == 'brati':
            continue
        d = data.loc[(i, slice(None))]
        ln = plotfun(d)[0]
        ln.set_label(i)
        ln.set_color(aggregate['color'][i])
        ln.set_linestyle('-')
    _, ymax = plt.ylim()
    plt.ylim(0, ymax)
    # Use upper left anchor to align legend on the right side outside axes
    # because makes sense, right?
    #plt.legend(bbox_to_anchor=(1.0, 1.0), loc='upper left')
    #plt.legend(ncol=5, bbox_to_anchor=(0.5, 1.025), loc='upper center')
    #plt.legend(ncol=2)
    plt.xticks(rotation=90)

def export(filename, legend=True):
    path = os.path.join(outdir, filename)

    kwargs = {}
    if legend:
        lgd = plt.legend(loc='center left', bbox_to_anchor=(1.0, 0.5))
        kwargs['bbox_extra_artists'] = (lgd,)
    #plt.tight_layout()
    plt.savefig(path, bbox_inches='tight', **kwargs)

plt.figure()
plt.title('charcount')
doplot(monthly,
    lambda d: plt.plot_date(d.index[1:], d['charcount'].cumsum()[1:], marker=None))
export('charcount_overtime.svg')
plt.figure()
plt.title('charcount without brati')
doplot(monthly,
    lambda d: plt.plot_date(d.index[1:], d['charcount'].cumsum()[1:], marker=None),
    brati=False)
export('chars_overtime_nobrati.svg')

# WORKAROUND BECAUSE OLD MATPLOTLIB CANT HANDLE STRINGS IN INDEX!
def mybar(x, y, *args, **kwargs):
    x_numeric = list(range(len(x)))
    plt.bar(x_numeric, y, *args, **kwargs)
    plt.xticks(x_numeric, x)

plt.figure()
plt.title('charcount')
#plt.bar(aggregate_all.index, aggregate_all['charcount'], color=barcolors)
# PLS SAVE ME
mybar(aggregate_all.index, aggregate_all['charcount'], color=barcolors)
plt.xticks(rotation=90)
export('chars.svg', legend=False)

plt.figure()
ax = plt.axes()
p = ax.get_position()
ax.set_position([p.x0, p.y0, p.width*0.8, p.height])
plt.title('chars per month')
doplot(monthly, lambda d: plt.plot_date(d.index[1:], d['charcount'][1:]))
export('charspermonth.svg')
plt.figure()
plt.title('chars per month')
doplot(monthly, lambda d: plt.plot_date(d.index[1:], d['charcount'][1:]), brati=False)
export('charspermonth_nobrati.svg')

plt.figure()
plt.title('chars per line')
doplot(monthly_late, lambda d: plt.plot_date(d.index[1:], (d['charcount'] / d['linecount'])[1:], linestyle='-'))
export('charsperlinepermonth.svg')

plt.figure()
plt.title('chars per line')
mybar(aggregate.index, aggregate['charcount'] / aggregate['linecount'], color=barcolors)
#plt.bar(aggregate.index, aggregate['charcount'] / aggregate['linecount'], color=barcolors)
plt.xticks(rotation=90)
export('charsperline.svg', legend=False)

plt.figure()
plt.title('chars per word')
mybar(aggregate.index, aggregate['charcount'] / aggregate['wordcount'], color=barcolors)
plt.xticks(rotation=90)
export('charsperword.svg', legend=False)

plt.figure()
plt.title('how much of this pie chart is a pie chart')
plt.pie([1.0, 0.0])
plt.legend(labels=['pie chart', 'not pie chart'])
export('piechart.svg', legend=False)

