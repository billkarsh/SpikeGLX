# SpikeGLX Broken Shank Support

**Topics**:

* [Shift Register Errors](#shift-register-errors)
* [Prior Policy](#prior-policy)
* [New Policy](#new-policy)
* [New Policy Implementation](#new-policy-implementation)
    + [Mandatory BIST-SR](#mandatory-bist-sr)
    + [IMRO Editor Cue](#imro-editor-cue)
    + [Join Tips](#join-tips)
    + [Updated Validation](#updated-validation)
    + [Updated Programming](#updated-programming)
    + [Survey Mode](#survey-mode)
* [Quad-base File Saving](#quad-base-file-saving)
* [Pitfalls and Caveats](#pitfalls-and-caveats)
    + [Undetected Errors](#undetected-errors)
    + [Reference Noise](#reference-noise)


# Shift Register Errors

A shift register is a circuit element that stores a value. When you load
a new value into it, the old value is pushed out:

```
new-->| value |-->old
```

You can chain these elements together. For example, suppose three elements
are currently storing values {X Y Z}. We can use three pushes to replace
that with {A B C}:

```
Start:  ->| X |->| Y |->| Z |->
Push C: ->| C |->| X |->| Y |-> collected {Z}
Push B: ->| B |->| C |->| X |-> collected {Y Z}
Push A: ->| A |->| B |->| C |-> collected {X Y Z}
```

We can test if we successfully loaded {A B C} into the registers by pushing
{A B C} twice. On pass 1 we will collect the original data, as demonstrated
for {X Y Z}, but that does not tell us about {A B C}. However, if we push
{A B C} again, we should collect {A B C}. If we collect anything else then
something is not working. That's a shift register chain error. This is how
Neuropixels probes program electrode and reference selections into a shank.
The first pass programs shank registers, the second pass checks for error.

At present, SR-chain error checking is unsophisticated: either there is a
perfect match, or an SR-chain error is reported for that shank. There is
no additional detail available about what elements of the chain are working
or if the chain is severed. That is, it is possible that some registers
hold the intended values, but it is possible that none of them do. We can
not tell.

--------

# Prior Policy

Prior to SpikeGLX version 20250930 we had a very conservative policy
regarding data integrity. We would always program your electrode and
reference selections with full error checking enabled. If **any** error
occurred (SR-chain or otherwise) at runtime, you would get an error
message about the type of failure, and the run would be prevented.

>Note: NP1.0-like probes {1.0, NHP, UHD} do not trigger SR-chain errors
at runtime. That is, even if the BIST SR test detects an error, this is
not detected when you click run, which is why it is a very good idea
when using these probes to check the box called `Include probe checks`
next to the `Detect` button (for SpikeGLX prior to 20250930).

--------

# New Policy

With SpikeGLX version 20250930 we adopt a more permissive runtime policy
regarding SR-chain errors that depends upon the probe type and circumstances.
The policy regarding errors that are not SR-chain errors remains the same:
if any non-SR error occurs we tell you and prevent running.

There are three types of probes with respect to SR error policy, assuming,
there are no other errors for the probe:

1. **Single-shank probes**: If a single-shank probe has an SR error there is
no workaround available. We prevent running in this case. For NP1.0-like
probes the new policy is actually more strict than before as we prevent
running based upon the BIST SR test, even though the runtime checks do not
report this issue.

2. **Standard multishank**: If a 2.0, non-quad-base multishank probe has
at least one working shank, we permit programming and running. In this
case we enforce that you make electrode and reference selections only on
the good shanks. 

2. **Quad-base**: If a QB probe has at least one working shank, we permit
programming and running. Note that it is not possible to simply allocate
all channels to the good shanks because the shanks are independent. You
must program something for each shank, and it remains your burden to be
highly suspicious of the data from the bad shanks.

--------

## New Policy Implementation

### Mandatory BIST-SR

Previously SpikeGLX *suggested it was useful* for you to check the box
called `Include probe checks` so that, upon clicking `Detect`, you would
get an early notification of any SR issues. That also governed whether to
do a PSB BIST to check the health of your flex-headstage connections.

Now, that check box is *only* an election whether to do the PSB test upon
`Detect`. The SR BIST is now mandatory so that we can assess that issue and
enforce appropriate policy.

SR tests results are obtained for each probe at `Detect` and become part of
the metadata for the probe, as is done for their part- and serial-numbers.
These metadata are recorded in the run .meta files.

### IMRO Editor Cue

The graphical IMRO editors behave the same, but a red `X` is drawn through
any bad shank. The editors offer this visual reminder, but do not themselves
alter any IMRO selections.

### Join Tips

This remains a valid option with broken shanks: only the good shanks are
connected together. This results in a smaller overall reference area, so
cancellation will not be as good. Also review [caveats below](#reference-noise).

### Updated Validation

In the Acquisition (Run) Configuration dialog, when you click `Verify` or
`Run`, as before, a series of self-consistency and sanity checks are applied
to all your settings.

These checks now implement the new policy outlined above. You are notified
that running is not possible, or that you should edit your IMRO selections
to permit running with the good shanks.

### Updated Programming

There are two probe configuration API calls that are handled differently
with the new policy:

1. Init(): After communication with a probe is established, this call sets
several default parameters, and it is possible that this can fail due to a
SR-chain error. However, we will overlook that error type if it is already
established at `Detect` that this probe has an SR error. We will not ignore
the error if the probe showed no previous SR error at `Detect`.

2. writeConfiguration(): The collected electrode, reference and other
settings are downloaded to the probe via the writeConfiguration() API
call. This call has two selectable modes of operation. The full error
checking mode is used for any probe that passed all SR tests at `Detect`.
Full error checking entails programming the SR chains twice to check for
an exact input/output match as described above. Alternatively, if the
probe has SR errors at `Detect` we employ the other mode which programs
each SR-chain on the probe just once, that is, **error checks are disabled**.

### Survey Mode

You can run probe surveys to sample activity on the entirety of the good
shanks, as assessed at `Detect`. SpikeGLX survey acquisition and analysis
bad-shank aware.

>*For those writing their own survey analysis code: The survey
bank-transition metadata item `~svySBTT` gets a prepended element giving
the starting (shank,bank) for the survey, which can no longer be assumed
to be (0,0).*

--------

## Quad-base File Saving

With QB probes you must acquire 384 channels from each shank since the
probe hardware acts like four single-shank probes. Therefore, you must
select sites on each shank whether good or bad.

On the other hand, you do not have to save bad shanks to runtime binary
files. To exclude channels on bad shanks follow these steps in the
IMRO editor:

1. Make your channel selections in the usual way on the good shanks.
2. Clear all channels from the bad shanks.
3. Click `Boxes => file chans`. The Save dialog will show with the list
preset to channels on good shanks.
4. Click `OK`.
5. In the IMRO editor, set `Nrows`=192 and `New boxes`=`Full shank`.
6. Click a single box of 384 sites on each bad shank.
7. Save the IMRO file for the next run.

--------

## Pitfalls and Caveats

### Undetected Errors

On the probe ASIC (base) there are shank SR-chains, and there are base
SR-chains that handle other key probe settings. In order to run a probe
with damaged shank SR-chains we must disable **all** SR error checking
in the writeConfiguration() call. If any error occurs while programming
either the good shank SR-chains or the base SR-chains it will not be
reported.

When a probe has sustained damage, understand that the damage could be more
extensive than you think. The permissive policy allows you to run, but can
no longer guarantee the data are free from error.

### Reference Noise

SpikeGLX will guide you to make electrode and reference selections on the
good shanks. However, the bad shanks have some influence on the quality
of the reference. The shanks contain switches to connect the various ref
sources onto the reference lines to the differential amplifiers. So, for
example, join tips works by closing a tip switch on each of the shanks.
However, on the bad shanks, we are no longer able to control those switches.
So regardless of the desired/set referencing mode, the bad shanks could be
coupling in unmanaged external or tip sources.

Expect noise pollution in the reference. This may vary in an uncontrolled
way from run to run.


_fin_

