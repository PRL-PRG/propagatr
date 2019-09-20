#ifndef PROMISEDYNTRACER_EVENT_H
#define PROMISEDYNTRACER_EVENT_H

#include <string>

enum class Event {
    DyntraceEntry = 0,
    DyntraceExit,
    DeserializeObject,
    EvalEntry,
    ArgumentListCreationEntry,
    ArgumentListCreationExit,
    ClosureEntry,
    ClosureExit,
    BuiltinEntry,
    BuiltinExit,
    SpecialEntry,
    SpecialExit,
    Substitute,
    S3DispatchEntry,
    S4DispatchArgument,
    ContextEntry,
    ContextJump,
    ContextExit,
    GcAllocate,
    PromiseForceEntry,
    PromiseForceExit,
    PromiseValueLookup,
    PromiseExpressionLookup,
    PromiseEnvironmentLookup,
    PromiseExpressionAssign,
    PromiseValueAssign,
    PromiseEnvironmentAssign,
    PromiseSubstitute,
    GcUnmark,
    GcEntry,
    EnvironmentVariableDefine,
    EnvironmentVariableAssign,
    EnvironmentVariableRemove,
    EnvironmentVariableLookup,
    EnvironmentContextSensitivePromiseEvalEntry,
    EnvironmentContextSensitivePromiseEvalExit,
    COUNT
};

inline std::string to_string(const Event event) {
    switch (event) {
    case Event::DyntraceEntry:
        return "DyntraceEntry";
    case Event::DyntraceExit:
        return "DyntraceExit";
    case Event::DeserializeObject:
        return "DeserializeObject";
    case Event::EvalEntry:
        return "EvalEntry";
    case Event::ArgumentListCreationEntry:
        return "ArgumentListCreationEntry";
    case Event::ArgumentListCreationExit:
        return "ArgumentListCreationExit";
    case Event::ClosureEntry:
        return "ClosureEntry";
    case Event::ClosureExit:
        return "ClosureExit";
    case Event::BuiltinEntry:
        return "BuiltinEntry";
    case Event::BuiltinExit:
        return "BuiltinExit";
    case Event::SpecialEntry:
        return "SpecialEntry";
    case Event::SpecialExit:
        return "SpecialExit";
    case Event::Substitute:
        return "Substitute";
    case Event::S3DispatchEntry:
        return "S3DispatchEntry";
    case Event::S4DispatchArgument:
        return "S4DispatchArgument";
    case Event::ContextEntry:
        return "ContextEntry";
    case Event::ContextJump:
        return "ContextJump";
    case Event::ContextExit:
        return "ContextExit";
    case Event::GcAllocate:
        return "GcAllocate";
    case Event::PromiseForceEntry:
        return "PromiseForceEntry";
    case Event::PromiseForceExit:
        return "PromiseForceExit";
    case Event::PromiseValueLookup:
        return "PromiseValueLookup";
    case Event::PromiseExpressionLookup:
        return "PromiseExpressionLookup";
    case Event::PromiseEnvironmentLookup:
        return "PromiseEnvironmentLookup";
    case Event::PromiseExpressionAssign:
        return "PromiseExpressionAssign";
    case Event::PromiseValueAssign:
        return "PromiseValueAssign";
    case Event::PromiseEnvironmentAssign:
        return "PromiseEnvironmentAssign";
    case Event::PromiseSubstitute:
        return "PromiseSubstitute";
    case Event::GcUnmark:
        return "GcUnmark";
    case Event::GcEntry:
        return "GcEntry";
    case Event::EnvironmentVariableDefine:
        return "EnvironmentVariableDefine";
    case Event::EnvironmentVariableAssign:
        return "EnvironmentVariableAssign";
    case Event::EnvironmentVariableRemove:
        return "EnvironmentVariableRemove";
    case Event::EnvironmentVariableLookup:
        return "EnvironmentVariableLookup";
    case Event::EnvironmentContextSensitivePromiseEvalEntry:
        return "EnvironmentContextSensitivePromiseEvalEntry";
    case Event::EnvironmentContextSensitivePromiseEvalExit:
        return "EnvironmentContextSensitivePromiseEvalExit";
    case Event::COUNT:
        return "UnknownEvent";
    }

    return "UnknownEvent";
}

#endif /* PROMISEDYNTRACER_EVENT_H */
