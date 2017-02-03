# SpikeGLX FAQ

**Topics:**

* [Data Integrity]

## Data Integrity

"My run quit unexpectedly, are my data likely to be corrupt or garbage?"

#### Graceful Shutdown

SpikeGLX monitors a number of health and performance metrics during a
data taking run. If there are signs of **pending** trouble it will
initiate a graceful shutdown of the run before a catastrophic failure
occurs. It closes open data files and then stops the data acquisition.
Messages in the Console window's log will describe the specific problem
encountered and the fact that the run was stopped. Your data files are
intact because we close them before corruption happens.

#### Crash

A graceful shutdown, described above, is **not a "crash"**. Software
engineers reserve the term "crash" for a completely pathological and
unexpected event that is so bad, the operating system must step in
and terminate the application with extreme prejudice before harm is
done to other programs or to the OS itself.

What are the signs of a crash? When an event like this happens, there
will usually be a Windows OS dialog box on the screen with a cryptic
message about quitting unexpectedly. Often, since Windows 7, the screen
has a peculiar milky appearance after a crash. A really severe crash ends
in the blue screen of death.

Crashes are usually the consequence of software bugs (bad practice) and
these mistakes are fixed over time as they're uncovered. If a crash happens
while running SpikeGLX (unlikely), you're still in pretty good shape
because the data files will be intact up to the moment just before the
crash. The crash itself will be very swift, so only the last few data
points in the files may be in question.

<br>
_fin_



