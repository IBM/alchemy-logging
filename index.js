const bunyan = require('bunyan');

/////////////////////////////////////////////////////////////////////
// Bunyan's built in log levels include entries for the following:
// 10 -> trace, 20 -> debug, 30 -> info, 40 -> warn, 50 -> error, 60 -> fatal
// Make sure there aren't any collisions!
const OFF = 60;
const DEBUG1 = 9;
const DEBUG2 = 8;
const DEBUG3 = 7;
const DEBUG4 = 6;

// Build the map which maps strings -> numeric levels
const customLevelFromName  = {
    'off': OFF,
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
    Object.assign(bunyan.levelFromName, customLevelFromName);
    Object.assign(bunyan.nameFromLevel, customNameFromLevel);
}

configure();
sampleLogger = bunyan.createLogger({name: 'test_app'});

// Put these things into a real testing dir if any of this actually works. Bunyan should be able to resolve custom levels.
bunyan.resolveLevel('debug1');

// And we also should be able to actually call these things as functions [Fails miserably]
sampleLogger.debug1('I am a custom level that is being tested!');
