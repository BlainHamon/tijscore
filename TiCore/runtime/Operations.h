/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2002, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef Operations_h
#define Operations_h

#include "ExceptionHelpers.h"
#include "Interpreter.h"
#include "JSImmediate.h"
#include "JSNumberCell.h"
#include "TiString.h"

namespace TI {

    NEVER_INLINE TiValue jsAddSlowCase(CallFrame*, TiValue, TiValue);
    TiValue jsTypeStringForValue(CallFrame*, TiValue);
    bool jsIsObjectType(TiValue);
    bool jsIsFunctionType(TiValue);

    ALWAYS_INLINE TiValue jsString(TiExcState* exec, TiString* s1, TiString* s2)
    {
        unsigned length1 = s1->length();
        if (!length1)
            return s2;
        unsigned length2 = s2->length();
        if (!length2)
            return s1;
        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);

        unsigned fiberCount = s1->size() + s2->size();
        TiGlobalData* globalData = &exec->globalData();

        if (fiberCount <= TiString::s_maxInternalRopeLength)
            return new (globalData) TiString(globalData, fiberCount, s1, s2);

        TiString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);
        ropeBuilder.append(s1);
        ropeBuilder.append(s2);
        return new (globalData) TiString(globalData, ropeBuilder.release());
    }

    ALWAYS_INLINE TiValue jsString(TiExcState* exec, const UString& u1, TiString* s2)
    {
        unsigned length1 = u1.size();
        if (!length1)
            return s2;
        unsigned length2 = s2->length();
        if (!length2)
            return jsString(exec, u1);
        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);

        unsigned fiberCount = 1 + s2->size();
        TiGlobalData* globalData = &exec->globalData();

        if (fiberCount <= TiString::s_maxInternalRopeLength)
            return new (globalData) TiString(globalData, fiberCount, u1, s2);

        TiString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);
        ropeBuilder.append(u1);
        ropeBuilder.append(s2);
        return new (globalData) TiString(globalData, ropeBuilder.release());
    }

    ALWAYS_INLINE TiValue jsString(TiExcState* exec, TiString* s1, const UString& u2)
    {
        unsigned length1 = s1->length();
        if (!length1)
            return jsString(exec, u2);
        unsigned length2 = u2.size();
        if (!length2)
            return s1;
        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);

        unsigned fiberCount = s1->size() + 1;
        TiGlobalData* globalData = &exec->globalData();

        if (fiberCount <= TiString::s_maxInternalRopeLength)
            return new (globalData) TiString(globalData, fiberCount, s1, u2);

        TiString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);
        ropeBuilder.append(s1);
        ropeBuilder.append(u2);
        return new (globalData) TiString(globalData, ropeBuilder.release());
    }

    ALWAYS_INLINE TiValue jsString(TiExcState* exec, const UString& u1, const UString& u2)
    {
        unsigned length1 = u1.size();
        if (!length1)
            return jsString(exec, u2);
        unsigned length2 = u2.size();
        if (!length2)
            return jsString(exec, u1);
        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);

        TiGlobalData* globalData = &exec->globalData();
        return new (globalData) TiString(globalData, u1, u2);
    }

    ALWAYS_INLINE TiValue jsString(TiExcState* exec, const UString& u1, const UString& u2, const UString& u3)
    {
        unsigned length1 = u1.size();
        unsigned length2 = u2.size();
        unsigned length3 = u3.size();
        if (!length1)
            return jsString(exec, u2, u3);
        if (!length2)
            return jsString(exec, u1, u3);
        if (!length3)
            return jsString(exec, u1, u2);

        if ((length1 + length2) < length1)
            return throwOutOfMemoryError(exec);
        if ((length1 + length2 + length3) < length3)
            return throwOutOfMemoryError(exec);

        TiGlobalData* globalData = &exec->globalData();
        return new (globalData) TiString(globalData, u1, u2, u3);
    }

    ALWAYS_INLINE TiValue jsString(TiExcState* exec, Register* strings, unsigned count)
    {
        ASSERT(count >= 3);

        unsigned fiberCount = 0;
        for (unsigned i = 0; i < count; ++i) {
            TiValue v = strings[i].jsValue();
            if (LIKELY(v.isString()))
                fiberCount += asString(v)->size();
            else
                ++fiberCount;
        }

        TiGlobalData* globalData = &exec->globalData();
        if (fiberCount == 3)
            return new (globalData) TiString(exec, strings[0].jsValue(), strings[1].jsValue(), strings[2].jsValue());

        TiString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);

        unsigned length = 0;
        bool overflow = false;

        for (unsigned i = 0; i < count; ++i) {
            TiValue v = strings[i].jsValue();
            if (LIKELY(v.isString()))
                ropeBuilder.append(asString(v));
            else
                ropeBuilder.append(v.toString(exec));

            unsigned newLength = ropeBuilder.length();
            if (newLength < length)
                overflow = true;
            length = newLength;
        }

        if (overflow)
            return throwOutOfMemoryError(exec);

        return new (globalData) TiString(globalData, ropeBuilder.release());
    }

    ALWAYS_INLINE TiValue jsString(TiExcState* exec, TiValue thisValue, const ArgList& args)
    {
        unsigned fiberCount = 0;
        if (LIKELY(thisValue.isString()))
            fiberCount += asString(thisValue)->size();
        else
            ++fiberCount;
        for (unsigned i = 0; i < args.size(); ++i) {
            TiValue v = args.at(i);
            if (LIKELY(v.isString()))
                fiberCount += asString(v)->size();
            else
                ++fiberCount;
        }

        TiString::RopeBuilder ropeBuilder(fiberCount);
        if (UNLIKELY(ropeBuilder.isOutOfMemory()))
            return throwOutOfMemoryError(exec);

        if (LIKELY(thisValue.isString()))
            ropeBuilder.append(asString(thisValue));
        else
            ropeBuilder.append(thisValue.toString(exec));

        unsigned length = 0;
        bool overflow = false;

        for (unsigned i = 0; i < args.size(); ++i) {
            TiValue v = args.at(i);
            if (LIKELY(v.isString()))
                ropeBuilder.append(asString(v));
            else
                ropeBuilder.append(v.toString(exec));

            unsigned newLength = ropeBuilder.length();
            if (newLength < length)
                overflow = true;
            length = newLength;
        }

        if (overflow)
            return throwOutOfMemoryError(exec);

        TiGlobalData* globalData = &exec->globalData();
        return new (globalData) TiString(globalData, ropeBuilder.release());
    }

    // ECMA 11.9.3
    inline bool TiValue::equal(TiExcState* exec, TiValue v1, TiValue v2)
    {
        if (v1.isInt32() && v2.isInt32())
            return v1 == v2;

        return equalSlowCase(exec, v1, v2);
    }

    ALWAYS_INLINE bool TiValue::equalSlowCaseInline(TiExcState* exec, TiValue v1, TiValue v2)
    {
        do {
            if (v1.isNumber() && v2.isNumber())
                return v1.uncheckedGetNumber() == v2.uncheckedGetNumber();

            bool s1 = v1.isString();
            bool s2 = v2.isString();
            if (s1 && s2)
                return asString(v1)->value(exec) == asString(v2)->value(exec);

            if (v1.isUndefinedOrNull()) {
                if (v2.isUndefinedOrNull())
                    return true;
                if (!v2.isCell())
                    return false;
                return v2.asCell()->structure()->typeInfo().masqueradesAsUndefined();
            }

            if (v2.isUndefinedOrNull()) {
                if (!v1.isCell())
                    return false;
                return v1.asCell()->structure()->typeInfo().masqueradesAsUndefined();
            }

            if (v1.isObject()) {
                if (v2.isObject())
                    return v1 == v2;
                TiValue p1 = v1.toPrimitive(exec);
                if (exec->hadException())
                    return false;
                v1 = p1;
                if (v1.isInt32() && v2.isInt32())
                    return v1 == v2;
                continue;
            }

            if (v2.isObject()) {
                TiValue p2 = v2.toPrimitive(exec);
                if (exec->hadException())
                    return false;
                v2 = p2;
                if (v1.isInt32() && v2.isInt32())
                    return v1 == v2;
                continue;
            }

            if (s1 || s2) {
                double d1 = v1.toNumber(exec);
                double d2 = v2.toNumber(exec);
                return d1 == d2;
            }

            if (v1.isBoolean()) {
                if (v2.isNumber())
                    return static_cast<double>(v1.getBoolean()) == v2.uncheckedGetNumber();
            } else if (v2.isBoolean()) {
                if (v1.isNumber())
                    return v1.uncheckedGetNumber() == static_cast<double>(v2.getBoolean());
            }

            return v1 == v2;
        } while (true);
    }

    // ECMA 11.9.3
    ALWAYS_INLINE bool TiValue::strictEqualSlowCaseInline(TiExcState* exec, TiValue v1, TiValue v2)
    {
        ASSERT(v1.isCell() && v2.isCell());

        if (v1.asCell()->isString() && v2.asCell()->isString())
            return asString(v1)->value(exec) == asString(v2)->value(exec);

        return v1 == v2;
    }

    inline bool TiValue::strictEqual(TiExcState* exec, TiValue v1, TiValue v2)
    {
        if (v1.isInt32() && v2.isInt32())
            return v1 == v2;

        if (v1.isNumber() && v2.isNumber())
            return v1.uncheckedGetNumber() == v2.uncheckedGetNumber();

        if (!v1.isCell() || !v2.isCell())
            return v1 == v2;

        return strictEqualSlowCaseInline(exec, v1, v2);
    }

    ALWAYS_INLINE bool jsLess(CallFrame* callFrame, TiValue v1, TiValue v2)
    {
        if (v1.isInt32() && v2.isInt32())
            return v1.asInt32() < v2.asInt32();

        double n1;
        double n2;
        if (v1.getNumber(n1) && v2.getNumber(n2))
            return n1 < n2;

        TiGlobalData* globalData = &callFrame->globalData();
        if (isTiString(globalData, v1) && isTiString(globalData, v2))
            return asString(v1)->value(callFrame) < asString(v2)->value(callFrame);

        TiValue p1;
        TiValue p2;
        bool wasNotString1 = v1.getPrimitiveNumber(callFrame, n1, p1);
        bool wasNotString2 = v2.getPrimitiveNumber(callFrame, n2, p2);

        if (wasNotString1 | wasNotString2)
            return n1 < n2;

        return asString(p1)->value(callFrame) < asString(p2)->value(callFrame);
    }

    inline bool jsLessEq(CallFrame* callFrame, TiValue v1, TiValue v2)
    {
        if (v1.isInt32() && v2.isInt32())
            return v1.asInt32() <= v2.asInt32();

        double n1;
        double n2;
        if (v1.getNumber(n1) && v2.getNumber(n2))
            return n1 <= n2;

        TiGlobalData* globalData = &callFrame->globalData();
        if (isTiString(globalData, v1) && isTiString(globalData, v2))
            return !(asString(v2)->value(callFrame) < asString(v1)->value(callFrame));

        TiValue p1;
        TiValue p2;
        bool wasNotString1 = v1.getPrimitiveNumber(callFrame, n1, p1);
        bool wasNotString2 = v2.getPrimitiveNumber(callFrame, n2, p2);

        if (wasNotString1 | wasNotString2)
            return n1 <= n2;

        return !(asString(p2)->value(callFrame) < asString(p1)->value(callFrame));
    }

    // Fast-path choices here are based on frequency data from SunSpider:
    //    <times> Add case: <t1> <t2>
    //    ---------------------------
    //    5626160 Add case: 3 3 (of these, 3637690 are for immediate values)
    //    247412  Add case: 5 5
    //    20900   Add case: 5 6
    //    13962   Add case: 5 3
    //    4000    Add case: 3 5

    ALWAYS_INLINE TiValue jsAdd(CallFrame* callFrame, TiValue v1, TiValue v2)
    {
        double left = 0.0, right;
        if (v1.getNumber(left) && v2.getNumber(right))
            return jsNumber(callFrame, left + right);
        
        if (v1.isString()) {
            return v2.isString()
                ? jsString(callFrame, asString(v1), asString(v2))
                : jsString(callFrame, asString(v1), v2.toPrimitiveString(callFrame));
        }

        // All other cases are pretty uncommon
        return jsAddSlowCase(callFrame, v1, v2);
    }

    inline size_t normalizePrototypeChain(CallFrame* callFrame, TiValue base, TiValue slotBase, const Identifier& propertyName, size_t& slotOffset)
    {
        TiCell* cell = asCell(base);
        size_t count = 0;

        while (slotBase != cell) {
            TiValue v = cell->structure()->prototypeForLookup(callFrame);

            // If we didn't find slotBase in base's prototype chain, then base
            // must be a proxy for another object.

            if (v.isNull())
                return 0;

            cell = asCell(v);

            // Since we're accessing a prototype in a loop, it's a good bet that it
            // should not be treated as a dictionary.
            if (cell->structure()->isDictionary()) {
                asObject(cell)->flattenDictionaryObject();
                if (slotBase == cell)
                    slotOffset = cell->structure()->get(propertyName); 
            }

            ++count;
        }
        
        ASSERT(count);
        return count;
    }

    inline size_t normalizePrototypeChain(CallFrame* callFrame, TiCell* base)
    {
        size_t count = 0;
        while (1) {
            TiValue v = base->structure()->prototypeForLookup(callFrame);
            if (v.isNull())
                return count;

            base = asCell(v);

            // Since we're accessing a prototype in a loop, it's a good bet that it
            // should not be treated as a dictionary.
            if (base->structure()->isDictionary())
                asObject(base)->flattenDictionaryObject();

            ++count;
        }
    }

    ALWAYS_INLINE TiValue resolveBase(CallFrame* callFrame, Identifier& property, ScopeChainNode* scopeChain)
    {
        ScopeChainIterator iter = scopeChain->begin();
        ScopeChainIterator next = iter;
        ++next;
        ScopeChainIterator end = scopeChain->end();
        ASSERT(iter != end);

        PropertySlot slot;
        TiObject* base;
        while (true) {
            base = *iter;
            if (next == end || base->getPropertySlot(callFrame, property, slot))
                return base;

            iter = next;
            ++next;
        }

        ASSERT_NOT_REACHED();
        return TiValue();
    }
} // namespace TI

#endif // Operations_h
