# drugsim
**drugsim** is a program designed to simulate drug concentrations in real-time
 using custom pharmacokinetic values.

> [!NOTE]
> This is a remake of the previous one written in python.
> This version was made in C++ and adds a couple more features.

## How it works
The program prompts for pharmacokinetic values which will be used to calculate
 and display drug concentrations in real time to the terminal.

## Prerequisites
- c++20
- terminal supporting ANSI codes
- [boost](https://www.boost.org/doc/user-guide/getting-started.html)

## Usage
While in the program directory, use the `make` command to build an executable.

Example of building the executable and running the program:
```
$ make
$ ./drugsim --help
```

### User Input
Upon running the script, required values will be prompted for input.\
Simulation assumes IV route of administration, this prompts the following:
```text
dose:
half-life:
```

Values can alternatively be given via cmd args, required info not provided via args will
 still be prompted.

Fractions and percent can also be used, e.g. `44%` would become `0.44`, and `1/2` would become `0.5`.

#### Time Units
When inputting time, the space between the time and unit is optional.\
If no units are provided, seconds will be used.

Available units:
- day(s)
  - Alias: d
- hour(s)
  - Alias: h, hr(s)
- minute(s)
  - Alias: m, min
- second(s)
  - Alias: s, sec
- millisecond(s)
  - Alias: ms

Examples:
```text
9 hours
5 hr
```

#### Dose Units
Like time units, the space between the number and unit are optional.\
If no units are provided, milligrams will be assumed (units will not be displayed if not provided).

Available units:
- micromolar(s)
  - Alias: uM, uMol
- nanomolar(s)
  - Alias: nM, nMol
- gram(s)
  - Alias: g
- milligram(s)
  - Alias: mg
- microgram(s)
  - Alias: mcg, ug
- nanogram(s)
  - Alias: ng
- Liter(s)
  - Alias: L
- milliliter(s)
  - Alias: mL

Dose units can also be given base units:
```text
1 mg/L
1000 mg/mL
1 mg/kg
```

##### Volume of Distribution
The volume of distribution can be given which will report the dose in concentration.\
This can be used by either entering a base using, e.g. `mg/L`, or by using the `volume` option:
```text
$ ./drugsim --volume=10
```

> [!NOTE]
> Volume option is in liters

If a dose of a mass is specified while using the `volume` option, the result will output concentration.

$$C_p = \frac{D \cdot F}{V}$$,\
 where $D$, $F$, and $V$ are dose, bioavailability, and volume, respectively.\
If dose were given as 200 mg and volume was given as 50, the result would be `4 mg/L`.

##### Precision and Significant Figures
The result can be given decimal precision and/or a significant figure to round to.

For precision:
```
$ ./drugsim -p3
```
If dose units are given and the precision is greater than 3, the units and precision will adjust, e.g.
once the dose drops below `1.000`, it will go from `0.999 mg` to `999 mcg`.

The last digit is rounded 5 up, i.e. `0.05 = 0.1`, once all are 0, i.e. `0.0...`, the program is considered complete.\
You can also set a minimum dose to be considered complete if prefered:
```
$ ./drugsim --min=10
```
Dose units are also supported by this, e.g. if minimum is 10 mg, 10 grams will not be considered complete.

> [!NOTE]
> If no minimum dose units are given, milligrams are assumed.

For sig figs:
```
$ ./drugsim --sigfigs=3
```

#### Route of Administration
Different routes of administration can be used via the `roa` argument:
```
$ ./drugsim --roa <route>
```

Available routes of administration:
- iv
- oral
  - Alias: po
- inhalation
  - Alias: inhale(d), smoke(d)
- intranasal
- sublingual
  - Alias: sl

#### IV
IV bolus will be used by default.

#### Oral
Changing the route of administration will also alter the values required to begin the simulation.\
Using oral administration, the prompt will look like this:
```text
dose:
bioavailability:
absorption half-life:
half-life:
```

##### Delayed Release
Delayed release drugs can be simulated where a specific fraction of the drug is
 released instantly, the remaining dose will be released after some time.\
For example, Adderall XR has 50% of the beads released instantly, the other half
 is intended to release 4 hours later.
```
$ ./drugsim --roa oral --dr 4h --irfrac 0.5 --msg 'adderall xr'
```

##### Prodrug
Prodrug and active drug concentrations can be simulated simultaneously.\
A progrug is an inactive (or less active) compound that is metabolized
 in the body into an active compound responsible for the therapeutic effects.

An example is lisdexamfetamine dimesylate (Vyvanse):
```
$ ./drugsim --roa oral --prodrug 0.297 -F 0.964 --msg='vyvanse'
```

The above example will display both prodrug and active drug concentrations.

#### Lagtime
The lagtime option will start a countdown before the simulation begins, this can
 account for the time it takes a drug to reach systemic circulation,
 e.g. pill ingestion.

For example, water takes 2&#8211;5 minutes to appear in plasma at measurable levels after ingestion.\
Illustrative example of how `lagtime` could be used:
```
$ ./drugsim --roa oral --dose '80 mg' --lagtime 2m --msg 'caffeine from coffee'
```

#### Timing
The simulation can start assuming administration occurred prior to the
 simulation commencement.

Start at a specific time:
```
$ ./drugsim --time 2056
$ ./drugsim --time '8:56 pm'
```

Administration at a specific date:
```
$ ./drugsim --date '20250403 0200'
$ ./drugsim --date '4/3/2025 2:00 am'
```

Elapsed since administration (e.g. 72 hours post-dose):
```
$ ./drugsim --elapsed 7200
$ ./drugsim --elapsed=72h
```

These options will take into account `lagtime` if used.

#### Effective Dose
$ED_{50}$ is the dose when it is 50% effective, effectiveness follows the formula:\
$$E(t) = \frac{1}{1 + \frac{ED_{50}}{C(t)}}$$

It will be displayed if the `ed50` arg is used, e.g.:
```
$ ./drugsim --ed50=5.8
```

#### Custom Messages
Custom messages can be used, e.g. you have multiple simulations running and
 want to keep track:
```
$ ./drugsim --msg='anything can go here'
```

#### Reading Files
Pharmacokinetic information can be stored in a json file to contain drug info:
```json
{
    "msg": "example.json",
    "roa": "sublingual",
    "dose": "200 ug"
}
```

```
$ ./drugsim --file=example
```

> [!NOTE]
> Custom pharmacokinetic configs must be in the same directory.
> JSON parsing is handled using [nlohmann/json](https://github.com/nlohmann/json/).

## See Also
https://en.wikipedia.org/wiki/Pharmacokinetics<br>
https://en.wikipedia.org/wiki/Monoamine_releasing_agent<br>
https://en.wikipedia.org/wiki/Monoamine_reuptake_inhibitor<br>
https://pmc.ncbi.nlm.nih.gov/articles/PMC3351614/<br>
