const bunyan = require('bunyan');

/////////////////////////////////////////////////////////////////////
// Bunyan's built in log levels include entries for the following:
// 10 -> trace, 20 -> debug, 30 -> info, 40 -> warn, 50 -> error, 60 -> fatal
// Make sure there aren't any collisions!
const OFF = 60;
const FATAL = 59;
const ERROR = bunyan.ERROR;
const WARNING = bunyan.WARN;
const INFO = bunyan.INFO;
const TRACE = 15;
const DEBUG = 10;
const DEBUG1 = 9;
const DEBUG2 = 8;
const DEBUG3 = 7;
const DEBUG4 = 6;

// Build the map which maps strings -> numeric levels
const customLevelFromName  = {
    'off': OFF,
    'fatal': FATAL,
    'error': ERROR,
    'warning': WARNING,
    'info': INFO,
    'trace': TRACE,
    'debug': DEBUG,
    'debug1': DEBUG1,
    'debug2': DEBUG2,
    'debug3': DEBUG3,
    'debug4': DEBUG4
};

// Build the map which maps numeric levels -> strings
const customNameFromLevel = {};
Object.keys(customLevelFromName).forEach(function(name) {
    customNameFromLevel[customLevelFromName[name]] = name;
});
/////////////////////////////////////////////////////////////////////


// STUB: Precondition check which blows up in a very angry way
// if any of our custom level (string keys or numeric values) explode if anything
// is conflicting with Bunyan.
function validateLevelMaps(customLevelFromName, customNameFromLevel) {
    // First, check for collisions against bunyan.levelFromName
    console.log(`STUB: Would have compared ${JSON.stringify(customLevelFromName)} to ${JSON.stringify(bunyan.levelFromName)} here.`);
    // Then, check against bunyan.nameFromLevel
    console.log(`STUB: Would have compared ${JSON.stringify(customNameFromLevel)} to ${JSON.stringify(bunyan.nameFromLevel)} here.`);
}

// Tweak the Bunyan log maps so that any logger created after this is called
// will have our fun custom levels.
function configure() {
    validateLevelMaps(customLevelFromName, customNameFromLevel);

    // Clear out all existing mappings from bunyan
    for (const property of Object.keys(bunyan.levelFromName)) {
        delete bunyan.levelFromName[property];
    }
    for (const property of Object.keys(bunyan.nameFromLevel)) {
        delete bunyan.nameFromLevel[property];
    }

    // Use our custom levels
    Object.assign(bunyan.levelFromName, customLevelFromName);
    Object.assign(bunyan.nameFromLevel, customNameFromLevel);
}

configure();
sampleLogger = bunyan.createLogger({name: 'test_app', level: DEBUG1});

// Put these things into a real testing dir if any of this actually works. Bunyan should be able to resolve custom levels.
bunyan.resolveLevel('debug1');




function mkALogEmitter(minLevel) {
    return function() {

        const record = {
            message: '',
            channel: this.fields.name.
            level: bunyan.nameFromLevel[this._level],
            timestamp: new Date(),
            num_indent: 0,
        };

        // If no args: log an empty message
        if (arguments.length === 0) {
            // Nothing to do
        }

        // If one arg:
        else if (arguments.length === 1) {
            // if function: invoke and log as message
            if (typeof arguments[0] === 'function') {

            }
            // elif objects: log key/val objects
            // else: log string message
        }

        // If two args:
        else if (arguments.length === 2) {
            // if first log code:
            //   set log code on props, treat second as single-arg message
            // else: first message, second additional key/val objects
        }

        // If three args:
        else if (arguments.length === 3) {
            // if first NOT a log code: barf
            // else: code, message, metadata
        }

        // More than three => error
        else {

        }
    }
}

// Feature Priorities //////////////////////////////////////////////////////////
/*

MUST HAVE:
* Full set of levels
* Multi-channel level setting
* Pretty and JSON formatting
* Lazy message construction
* (core detail) custom output stream, mostly for testing

CORE FEATURES:
* channel.with_metadata()
* Scoped indentation
* Function trace BEGIN/END
* Some way to produce complex logs lazily

NICE FEATURES:
* Scoped timing
* Decorators for scope behavior
* ALOG_USE_CHANNEL for classes

*/


// Examples ////////////////////////////////////////////////////////////////////

// const alog = require('alog');
alog.configure('debug', 'MAIN:debug4');
alog.configure('debug', { MAIN: 'debug4' });
alog.configure(alog.DEBUG, {MAIN: alog.DEBUG4});
alog.configure({
    defaultLevel: alog.DEBUG,
    filters: {
        MAIN: alog.DEBUG4
    },
    formatter: 'json' // 'pretty', CustomFormatter(),
    // threaId: true
});

const channel = alog.useChannel('MAIN');

// 1. Log code
// 2. Message generator (string, function, string + format args)
// 3. Metadata map
//
// log_code? message [format_args]...

channel.debug2('<TST12345678D>', 'This is the %dst test', 1);
channel.withMetadata({
    metakey: 'val',
}).info('<TST12345678I>', 'This is a test');
channel.debug4('<TST12345678D>', () => {
    let m = '';
    myContainer.forEach((element) => m += '--' + element);
    return m;
});
