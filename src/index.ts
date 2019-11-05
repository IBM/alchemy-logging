
// Third Party
import bunyan from 'bunyan';

// Constants ///////////////////////////////////////////////////////////////////

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
const customLevelFromName: {[levelName: string]: number} = {
    off: OFF,
    fatal: FATAL,
    error: ERROR,
    warning: WARNING,
    info: INFO,
    trace: TRACE,
    debug: DEBUG,
    debug1: DEBUG1,
    debug2: DEBUG2,
    debug3: DEBUG3,
    debug4: DEBUG4
};

function isLogCode(argOne: string) {
    return argOne.match(/^<[A-Z]{3}\d{8}[IWDEFT]>$/) !== null;
}

// Build the map which maps numeric levels -> strings
const customNameFromLevel: {[levelValue: number]: string} = {};
Object.keys(customLevelFromName).forEach((levelName: string) => {
    customNameFromLevel[customLevelFromName[levelName]] = levelName;
});

// Validation //////////////////////////////////////////////////////////////////

function isValidLevel(lvl: any) {
    return Number.isInteger(lvl) && lvl > 0;
}

function isValidFilterConfig(filterConfig: any) {
    for (const key of filterConfig) {
        if (!isValidLevel(filterConfig[key])) {
            return false;
        }
    }
    return true;
}

function isValidConfig(configObject: any): boolean {

    // defaultLevel
    if (!isValidLevel(configObject.defaultLevel)) {
        return false;
    }

    // filters
    if (configObject.filters !== undefined) {
        if (typeof configObject.filters !== 'object') {
            return false;
        }
        if (!isValidFilterConfig(configObject.filters)) {
            return false;
        }
    }

    // formatter
    if (configObject.formatter !== undefined && typeof configObject.formatter !== 'function') {
        return false;
    }

    // other

    return true;
}

// Formatter ///////////////////////////////////////////////////////////////////

function PrettyFormatter(record: any): string {
    //DEBUG
    return 'THIS IS A STUB !';
}

// Core Exports ////////////////////////////////////////////////////////////////

export type FormatterFunc = (logRecord: any) => string;

export interface AlogConfig {
    defaultLevel: number;
    filters?: {[channelName: string]: number};
    formatter?: FormatterFunc;
}

export class AlogConfigError extends Error {
    constructor(message: string) {
        super(message);
        this.name = this.constructor.name;
    }
};

// Tweak the Bunyan log maps so that any logger created after this is called
// will have our fun custom levels.
export function configure(argOne: AlogConfig | string, filters?: any, formatter?: string | FormatterFunc) {
    // Clear out all existing mappings from bunyan
    for (const property of Object.keys(bunyan.levelFromName)) {
        delete (bunyan.levelFromName as any)[property];
    }
    for (const property of Object.keys(bunyan.nameFromLevel)) {
        delete (bunyan.nameFromLevel as any)[property];
    }

    // Use our custom levels
    Object.assign(bunyan.levelFromName, customLevelFromName);
    Object.assign(bunyan.nameFromLevel, customNameFromLevel);

    // These are the three core pieces of config we need from the various args
    let parsedDefaultLevel: number = null;
    let parsedFilters: { [channelName: string]: number };
    let parsedFormatter: FormatterFunc = PrettyFormatter;

    // argOne might be a big map of all the things: (defaultLevel, Filters, formatter)
    let argOneObj = false;
    if (typeof argOne === "string") {
        if (bunyan.levelFromName.hasOwnProperty(argOne)) {
            parsedDefaultLevel = (bunyan.levelFromName as any)[argOne];
        } else {
            throw new AlogConfigError(`Invalid level string [${argOne}]`);
        }
    } else if (typeof argOne === "number") {
        if (isValidLevel(argOne)) {
            parsedDefaultLevel = argOne;
        } else {
            throw new AlogConfigError(`Invalid level value [${argOne}]`);
        }
    } else if (typeof argOne === "object") {
        argOneObj = true;
        if (isValidConfig(argOne)) {
            parsedDefaultLevel = argOne.defaultLevel;
            filters = argOne.filters || filters;
            formatter = argOne.formatter || formatter;
        } else {
            throw new AlogConfigError(`Invalid config object: ${JSON.stringify(argOne)}`);
        }
    } else {
        throw new AlogConfigError(`Invalid argument type: [${typeof argOne}]`);
    }

    // filters
    if (argOneObj && filters !== undefined) {
        throw new AlogConfigError('Cannot specify both config object and filters argument');
    } else if (filters !== undefined) {
        // Validate if it's an object
        if (typeof filters === 'object') {
            if (!isValidFilterConfig(filters)) {
                throw new AlogConfigError(`Invalid filter config: ${JSON.stringify(filters)}`);
            }
            parsedFilters = filters;
        } else if (typeof filters === 'string') {
            // Parse if it's a string
            const parsed: any = {};
            filters.split(',').forEach((part: string) => {
                const keyVal = part.split(':');
                if (keyVal.length !== 2) {
                    throw new AlogConfigError(`Invalid filter spec part [${part}]`);
                } else if (!isValidLevel(keyVal[1])) {
                    throw new AlogConfigError(`Invalid filter level [${part}]`);
                }
                parsed[keyVal[0]] = (bunyan.levelFromName as any)[keyVal[1]];
            });
            parsedFilters = parsed;
        } else {
            throw new AlogConfigError(`Invalid argument type for filters: [${typeof filters}]`);
        }
    }

    // formatter
}

// configure();
// sampleLogger = bunyan.createLogger({name: 'test_app', level: DEBUG1});

// // Put these things into a real testing dir if any of this actually works.
// Bunyan should be able to resolve custom levels.




function mkALogEmitter(minLevel: number) {
    return function() {

        const record = {
            message: '',
            channel: this.fields.name,
            level: bunyan.nameFromLevel[this._level],
            timestamp: new Date(),
            num_indent: 0,
        };

        // If no args: log an empty message
        if (arguments.length === 0) {
            // Nothing to do
        } else if (arguments.length === 1) {
        // If one arg:

            // if function: invoke and log as message
            if (typeof arguments[0] === 'function') {
                //DEBUG
            }
            // elif objects: log key/val objects
            // else: log string message
        } else if (arguments.length === 2) {
            // If two args:

            // if first log code:
            //   set log code on props, treat second as single-arg message
            // else: first message, second additional key/val objects
        } else if (arguments.length === 3) {
            // If three args:

            // if first NOT a log code: barf
            // else: code, message, metadata
        } else {
            // More than three => error

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

// // const alog = require('alog');
// alog.configure('debug', 'MAIN:debug4');
// alog.configure('debug', { MAIN: 'debug4' });
// alog.configure(alog.DEBUG, {MAIN: alog.DEBUG4});
// alog.configure({
//     defaultLevel: alog.DEBUG,
//     filters: {
//         MAIN: alog.DEBUG4
//     },
//     formatter: 'json' // 'pretty', CustomFormatter(),
//     // threaId: true
// });

// const channel = alog.useChannel('MAIN');

// // 1. Log code
// // 2. Message generator (string, function, string + format args)
// // 3. Metadata map
// //
// // log_code? message [format_args]...

// channel.debug2('<TST12345678D>', 'This is the %dst test', 1);
// channel.withMetadata({
//     metakey: 'val',
// }).info('<TST12345678I>', 'This is a test');
// channel.debug4('<TST12345678D>', () => {
//     let m = '';
//     myContainer.forEach((element) => m += '--' + element);
//     return m;
// });
