#include "strfunc.h"
#include <algorithm>
#include <format>
#include <string>
#include <vector>

extern std::vector<std::string> talks_;

std::string trans50(std::string source)
{
    strfunc::replaceAllSubStringRef(source, "\r", "");
    const std::vector<std::string> lines = strfunc::splitString(source, "\n", false);
    std::vector<std::string> outputLines;

    auto value = [](int bit, int flags, int value) -> std::string
    {
        return flags & (1 << bit) ? std::format("x[{}]", value) : std::to_string(value);
    };
    auto is_constant = [](int bit, int flags)
    {
        return (flags & (1 << bit)) == 0;
    };

    for (const auto& line : lines)
    {
        std::string output = line;
        const size_t instruction = line.find("instruct_50");
        if (instruction != std::string::npos)
        {
            constexpr std::string_view dynamicCodePrefix = "instruct_50e(x[";
            const size_t dynamicCodeStart = line.find(dynamicCodePrefix);
            if (dynamicCodeStart != std::string::npos)
            {
                const size_t dynamicCodeEnd = line.find(']', dynamicCodeStart + dynamicCodePrefix.size());
                const auto arguments = dynamicCodeEnd == std::string::npos
                    ? std::vector<int>()
                    : strfunc::findNumbers<int>(line.substr(dynamicCodeEnd + 1));
                if (arguments.size() >= 6)
                {
                    const std::string dynamicCode = line.substr(dynamicCodeStart + dynamicCodePrefix.size(), dynamicCodeEnd - dynamicCodeStart - dynamicCodePrefix.size());
                    output = std::format("switch (x[{}]) {{\n", dynamicCode);
                    for (int code = 0; code <= 56; ++code)
                    {
                        // These subcommands interpret e2 as a table selector or
                        // a talk index. An invalid branch is a no-op in the
                        // original dispatcher, and must not be expanded just
                        // because another runtime code may be selected.
                        if ((code == 8 && (arguments[1] < 0 || arguments[1] >= talks_.size())) ||
                            ((code == 16 || code == 17 || code == 27) && (arguments[1] < 0 || arguments[1] > 4)))
                        {
                            continue;
                        }
                        const std::string branch = trans50(std::format("instruct_50e({}, {}, {}, {}, {}, {}, {});", code,
                            arguments[0], arguments[1], arguments[2], arguments[3], arguments[4], arguments[5]));
                        // A subcommand with invalid selector arguments leaves
                        // its source text unchanged. Its runtime behavior is a
                        // no-op, so omit that switch branch instead of nesting
                        // a residual legacy call.
                        if (!branch.empty() && !branch.contains("instruct_50"))
                        {
                            output += std::format("case {}: {{ {} break; }}\n", code, branch);
                        }
                    }
                    output += "default: break;\n}";
                }
            }

            auto numbers = strfunc::findNumbers<int>(line.substr(instruction + 12));
            if (dynamicCodeStart == std::string::npos && numbers.size() >= 7)
            {
                const int code = numbers[0];
                const int flags = numbers[1];
                int e2 = numbers[2];
                int e3 = numbers[3];
                int e4 = numbers[4];
                int e5 = numbers[5];
                int e6 = numbers[6];

                switch (code)
                {
                case 0: output = std::format("x[{}] = {};", flags, e2); break;
                case 1:
                    output = std::format("x[{} + {}] = {};", e3, value(0, flags, e4), value(1, flags, e5));
                    if (e2) { output += std::format(" x[{} + {}] &= 255;", e3, value(0, flags, e4)); }
                    break;
                case 2:
                    output = std::format("x[{}] = x[{} + {}];", e5, e3, value(0, flags, e4));
                    if (e2) { output += std::format(" x[{}] &= 255;", e5); }
                    break;
                case 3:
                    if (e2 >= 0 && e2 <= 4)
                    {
                        static constexpr char operators[] = "+-*/%";
                        output = std::format("x[{}] = x[{}] {} {};", e3, e4, operators[e2], value(0, flags, e5));
                    }
                    else if (e2 == 5)
                    {
                        output = std::format("x[{}] = x[{}] / {};", e3, e4, value(0, flags, e5));
                    }
                    break;
                case 4:
                    if (e2 >= 0 && e2 <= 5)
                    {
                        static const std::vector<std::string> operators = { "<", "<=", "==", "!=", ">=", ">" };
                        output = std::format("x[28672] = !(x[{}] {} {});", e3, operators[e2], value(0, flags, e4));
                    }
                    else if (e2 == 6)
                    {
                        output = "x[28672] = false;";
                    }
                    else if (e2 == 7)
                    {
                        output = "x[28672] = true;";
                    }
                    break;
                case 5: output = "for (int i = 0; i <= 30000; i++) { x[i] = 0; }"; break;
                case 6:
                case 7:
                case 13:
                case 14:
                case 15:
                case 28:
                case 29:
                case 30:
                case 31:
                case 44:
                case 45:
                case 46:
                case 47:
                case 48:
                case 49:
                case 50:
                case 51:
                case 53:
                case 54:
                case 55:
                case 56:
                    output.clear();
                    break;
                case 8:
                    if (is_constant(0, flags))
                    {
                        output = std::format("x[{}] = \"{}\";", e3, talks_[e2]);
                    }
                    else
                    {
                        output = std::format("x[{}] = GetTalk({});", e3, value(0, flags, e2));
                    }
                    break;
                case 9: output = std::format("x[{}] = sprintf(x[{}], {});", e2, e3, value(0, flags, e4)); break;
                case 10: output = std::format("x[{}] = DrawLength(x[{}]);", e2, flags); break;
                case 11: output = std::format("x[{}] = x[{}] + x[{}];", e3, flags, e2); break;
                case 12: output = std::format("x[{}] = sprintf(\"%-*s\", {}, \"\");", e2, value(0, flags, e3)); break;
                case 16:
                {
                    static const std::vector<std::string> names = { "Role", "Item", "SubmapInfo", "Magic", "Shop" };
                    output = std::format("Set{}({}, {} / 2, {});", names[e2], value(0, flags, e3), value(1, flags, e4), value(2, flags, e5));
                    break;
                }
                case 17:
                {
                    static const std::vector<std::string> names = { "Role", "Item", "SubmapInfo", "Magic", "Shop" };
                    output = std::format("x[{}] = Get{}({}, {} / 2);", e5, names[e2], value(0, flags, e3), value(1, flags, e4));
                    break;
                }
                case 18: output = std::format("SetTeam({}, {});", value(0, flags, e2), value(1, flags, e3)); break;
                case 19: output = std::format("x[{}] = GetTeam({});", e3, value(0, flags, e2)); break;
                case 20: output = std::format("x[{}] = GetItemAmount({});", e3, value(0, flags, e2)); break;
                case 21: output = std::format("SetD({}, {}, {}, {});", value(0, flags, e2), value(1, flags, e3), value(2, flags, e4), value(3, flags, e5)); break;
                case 22: output = std::format("x[{}] = GetD({}, {}, {});", e5, value(0, flags, e2), value(1, flags, e3), value(2, flags, e4)); break;
                case 23: output = std::format("SetS({}, {}, {}, {}, {});", value(0, flags, e2), value(1, flags, e3), value(2, flags, e4), value(3, flags, e5), value(4, flags, e6)); break;
                case 24: output = std::format("x[{}] = GetS({}, {}, {}, {});", e6, value(0, flags, e2), value(1, flags, e3), value(2, flags, e4), value(3, flags, e5)); break;
                case 25:
                case 26:
                    output.clear();
                    break;
                case 27:
                {
                    static const std::vector<std::string> names = { "Role", "Item", "Submap", "Magic", "Shop" };
                    output = std::format("x[{}] = Get{}Name({});", e4, names[e2], value(0, flags, e3));
                    break;
                }
                case 32:
                    output.clear();
                    break;
                case 33: output = std::format("DrawString(x[{}], {}, {}, {});", e2, value(0, flags, e3), value(1, flags, e4), value(2, flags, e5)); break;
                case 34: output = std::format("DrawRect({}, {}, {}, {});", value(0, flags, e2), value(1, flags, e3), value(2, flags, e4), value(3, flags, e5)); break;
                case 35: output = std::format("x[{}] = GetKey();", flags); break;
                case 36: output = std::format("x[28672] = showmessage(x[{}], {}, {}, {});", e2, value(0, flags, e3), value(1, flags, e4), value(2, flags, e5)); break;
                case 37: output = std::format("Delay({});", value(0, flags, e2)); break;
                case 38: output = std::format("x[{}] = random({});", e3, value(0, flags, e2)); break;
                case 39:
                case 40:
                    output = std::format("strs = {{}}; for (int i = 1; i <= {}; i++) {{ strs[i] = x[x[{} + i - 1]]; }} x[{}] = menu({}, {}, strs, {});",
                        value(0, flags, e2), e3, e4, value(1, flags, e5), value(2, flags, e6), value(0, flags, e2));
                    break;
                case 41:
                    if (e2 == 0) { output = std::format("DrawMainImage({} / 2, {}, {});", value(2, flags, e5), value(0, flags, e3), value(1, flags, e4)); }
                    else if (e2 == 1) { output = std::format("DrawHeadImage({} / 2, {}, {});", value(2, flags, e5), value(0, flags, e3), value(1, flags, e4)); }
                    break;
                case 42:
                    output = std::format("SetMainMapPosition({}, {});", value(0, flags, e2), value(1, flags, e3));
                    break;
                case 43:
                    output = std::format("x[28928] = {}; x[28929] = {}; x[28930] = {}; x[28931] = {}; CallEvent({});",
                        value(1, flags, e3), value(2, flags, e4), value(3, flags, e5), value(4, flags, e6), value(0, flags, e2));
                    break;
                default: break;
                }
            }
        }
        if (!output.empty())
        {
            auto generatedLines = strfunc::splitString(output, "\n", false);
            outputLines.insert(outputLines.end(), generatedLines.begin(), generatedLines.end());
        }
    }

    auto trim = [](std::string value)
    {
        const auto first = value.find_first_not_of(" \t");
        if (first == std::string::npos) { return std::string(); }
        const auto last = value.find_last_not_of(" \t");
        return value.substr(first, last - first + 1);
    };

    for (auto& line : outputLines)
    {
        strfunc::replaceAllSubStringRef(line, "CheckRoleSexual(256)", "(x[28672] == false)");
    }

    // Case 4 stores the inverse comparison in x[28672]. The following legacy
    // CheckRoleSexual(256) branch consumes that flag, so inline the original
    // comparison instead of leaving an implementation-detail temporary.
    for (size_t i = 1; i < outputLines.size(); ++i)
    {
        const std::string previous = trim(outputLines[i - 1]);
        constexpr std::string_view prefix = "x[28672] = !(";
        if (!previous.starts_with(prefix) || !previous.ends_with(");") || !outputLines[i].contains("x[28672] == false"))
        {
            continue;
        }
        const std::string comparison = previous.substr(prefix.size(), previous.size() - prefix.size() - 2);
        strfunc::replaceAllSubStringRef(outputLines[i], "((x[28672] == false) == false)", "(!(" + comparison + "))");
        strfunc::replaceAllSubStringRef(outputLines[i], "(x[28672] == false)", "(" + comparison + ")");
        outputLines[i - 1].clear();
    }

    auto hasOnlyOneGoto = [&outputLines](const std::string& label)
    {
        const std::string target = "goto " + label;
        return std::count_if(outputLines.begin(), outputLines.end(), [&target](const std::string& line)
            { return line.contains(target); }) == 1;
    };

    for (auto& line : outputLines)
    {
        constexpr std::string_view prefix = "if (!(!(";
        if (line.starts_with(prefix))
        {
            const size_t conditionEnd = line.find(")))", prefix.size());
            if (conditionEnd != std::string::npos)
            {
                line = "if (" + line.substr(prefix.size(), conditionEnd - prefix.size()) + ")" + line.substr(conditionEnd + 3);
            }
        }
    }

    // A conditional forward goto with a unique target and no nested labels is a
    // straight-line false branch. Turn it into a Cifa if block so editors can fold it.
    for (size_t i = 0; i < outputLines.size(); ++i)
    {
        const std::string line = trim(outputLines[i]);
        if (!line.starts_with("if (") || !line.ends_with(";")) { continue; }

        const size_t conditionEnd = line.find(") goto ");
        if (conditionEnd == std::string::npos) { continue; }
        const std::string condition = line.substr(4, conditionEnd - 4);
        const std::string label = trim(line.substr(conditionEnd + 7, line.size() - conditionEnd - 8));
        if (label.empty() || !hasOnlyOneGoto(label)) { continue; }

        const std::string labelLine = label + ":";
        auto labelIt = std::find_if(outputLines.begin() + i + 1, outputLines.end(), [&labelLine, &trim](const std::string& candidate)
            { return trim(candidate) == labelLine; });
        if (labelIt == outputLines.end()) { continue; }

        const size_t labelIndex = size_t(labelIt - outputLines.begin());
        const bool hasNestedLabel = std::any_of(outputLines.begin() + i + 1, outputLines.begin() + labelIndex, [&trim](const std::string& candidate)
            { return trim(candidate).ends_with(":"); });
        if (hasNestedLabel) { continue; }

        outputLines[i] = std::format("if (!({})) {{", condition);
        outputLines[labelIndex] = "}";
    }

    std::string result;
    for (const auto& line : outputLines)
    {
        if (!trim(line).empty())
        {
            result += line + "\n";
        }
    }
    if (!result.empty()) { result.pop_back(); }
    return result;
}