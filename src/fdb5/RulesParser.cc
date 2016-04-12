/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @file   RulesParser.h
/// @author Baudouin Raoult
/// @author Tiago Quintino
/// @date   April 2016

#include "eckit/log/Log.h"

#include "fdb5/RulesParser.h"
#include "fdb5/Rule.h"
#include "fdb5/Predicate.h"
#include "fdb5/MatchAlways.h"
#include "fdb5/MatchAny.h"
#include "fdb5/MatchValue.h"

namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

class RulesTokenizerError : public std::exception {
    std::string what_;
    virtual const char* what() const  throw()
    {
        return what_.c_str();
    }
public:
    RulesTokenizerError(const std::string& what) : what_(what) {}
    virtual ~RulesTokenizerError() throw() {}
};

//----------------------------------------------------------------------------------------------------------------------

std::string RulesParser::parseIdent()
{
    std::string s;
    for(;;)
    {
        char c = peek();
        switch(c) {
            case 0:
            case '/':
            case '=':
            case ',':
            case '[':
            case ']':
                return s;

            default:
                consume(c);
                s += c;
                break;
        }
    }
}

Predicate* RulesParser::parsePredicate() {

    std::set<std::string> values;
    std::string k = parseIdent();

    char c = peek();

    if(c != ',' && c != '[' && c != ']')
    {
        consume("=");

        values.insert(parseIdent());

        while((c = peek()) == '/') {
            consume(c);
            values.insert(parseIdent());
        }
    }

    switch(values.size()) {
        case 0:
            return new Predicate(k, new MatchAlways());
            break;

        case 1:
            return new Predicate(k, new MatchValue(*values.begin()));
            break;

        default:
            return new Predicate(k, new MatchAny(values));
            break;
    }
}


Rule* RulesParser::parseRule()
{
    std::vector<Predicate*> predicates;
    std::vector<Rule*> rules;

    consume('[');
    char c = peek();
    if(c == ']')
    {
        consume(c);
        return new Rule(predicates, rules);
    }


    for(;;) {

        char c = peek();

        if( c == '[') {
            while( c == '[') {
                rules.push_back(parseRule());
                c = peek();
            }
        }
        else {
            predicates.push_back(parsePredicate());
            while( (c = peek()) == ',') {
                consume(c);
                predicates.push_back(parsePredicate());
            }
        }

        c = peek();
        if(c == ']')
        {
            consume(c);
            return new Rule(predicates, rules);
        }


    }
}

RulesParser::RulesParser(std::istream &in) : StreamParser(in)
{
}

void RulesParser::parse(std::vector<Rule*>& result)
{
    char c;
    while((c = peek()) == '[') {
        result.push_back(parseRule());
    }
    if(c) {
        throw RulesTokenizerError(std::string("Error parsing rules: remaining char: ") + c);
    }
}


//----------------------------------------------------------------------------------------------------------------------

} // namespace eckit