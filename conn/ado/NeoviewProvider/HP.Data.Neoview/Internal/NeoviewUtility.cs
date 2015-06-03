/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
********************************************************************/

namespace HP.Data
{
    using System;
    using System.Text.RegularExpressions;
    using System.Collections.Generic;
    using System.Data;

    /// <summary>
    /// Collection of static regular expressions, arrays, and methods used through out the provider.
    /// </summary>
    internal class HPDbUtility
    {
        /// <summary>
        /// Regular expression for validating numeric format.  Used in BigNum conversion.
        /// </summary>
        public static readonly Regex ValidateNumeric = new Regex("^-{0,1}\\d*\\.{0,1}\\d+$", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for trimming leading 0s.  Used in BigNum conversion.
        /// </summary>
        public static readonly Regex TrimNumericString = new Regex("0*", RegexOptions.Compiled);
        
        /// <summary>
        /// Regular expression for finding wild char characters in metadata parameters.
        /// </summary>
        public static readonly Regex FindWildCards = new Regex("[%_]", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for stripping out SQL comments.
        /// </summary>
        public static readonly Regex RemoveComments = new Regex("--.*[\r\n]|/\\*(.|[\r\n])*?\\*/", RegexOptions.Compiled);
        
        /// <summary>
        /// Regular expression for finding out particular num in the sql.
        /// </summary>
        public static readonly Regex FindRowSize = new Regex("[0-9]+", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for checking if the sql is RWRS User Load
        /// </summary>
        public static readonly Regex FindRowwiseUserLoad = new Regex(@"user\s+load", RegexOptions.Compiled);

        /// <summary>
        /// Regular expression for removing string literals.
        /// </summary>
        public static readonly Regex RemoveStringLiterals = new Regex("\"[^\"]*\"|'[^']*'", RegexOptions.Compiled);
        
        /// <summary>
        /// Regular expression for tokenizing words.
        /// </summary>
        public static readonly Regex Tokenize = new Regex("[^a-zA-Z]+", RegexOptions.Compiled);

        /// <summary>
        /// Most commonly used date format.
        /// </summary>
        public const string DateFormat = "yyyy-MM-dd";

        /// <summary>
        /// Commonly used time formats.   Precision 0 through 6.
        /// </summary>
        public static readonly string[] TimeFormat = 
        { 
            "HH:mm:ss", "HH:mm:ss.f", "HH:mm:ss.ff", "HH:mm:ss.fff", 
            "HH:mm:ss.ffff", "HH:mm:ss.fffff", "HH:mm:ss.ffffff" 
        };

        /// <summary>
        /// Commonly used Timestamp formats.  Precision 0 through 6
        /// </summary>
        public static readonly string[] TimestampFormat = 
        { 
            "yyyy-MM-dd HH:mm:ss", "yyyy-MM-dd HH:mm:ss.f", 
            "yyyy-MM-dd HH:mm:ss.ff", "yyyy-MM-dd HH:mm:ss.fff", 
            "yyyy-MM-dd HH:mm:ss.ffff", "yyyy-MM-dd HH:mm:ss.fffff", 
            "yyyy-MM-dd HH:mm:ss.ffffff" 
        };

        /// <summary>
        /// Array of powers of 10 that can be referenced by using the exponent as the offset
        /// </summary>
        public static readonly long[] PowersOfTen = 
        {
            1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 
            1000000000, 10000000000, 100000000000, 1000000000000,
            10000000000000, 100000000000000, 1000000000000000, 10000000000000000,
            100000000000000000, 1000000000000000000
        };

        /*
         * the following section is a hack implemented to work around that Visual Studio 2008 does not use the proper delimiter 
         * with its query builder.  We need to switch  [ ] to " before executing the statement without breaking 
         * string literals or valid SQL syntax
         * 
         * (""[^""]*"")                              -- find text enclosed in double quotes
         * ('[^']*')                                 -- find text enclosed in single quotes
         * ((\[)(FIRST|ANY|LAST)\s(\d)*?(\]))        -- find text in the form [<keyword> ####], keywords: FIRST, LAST, ANY  
         * 
         */
        
        /// <summary>
        /// Regular expression for removing valid brackets.
        /// </summary>
        public static readonly Regex RemoveValidBrackets = new Regex(@"(""[^""]*"")|('[^']*')|((\[)(FIRST|ANY|LAST)\s(\d)*?(\]))", RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Multiline);
        
        /// <summary>
        /// Regular expression for finding all the brackets.
        /// </summary>
        public static readonly Regex FindBrackets = new Regex(@"[\[\]]", RegexOptions.Compiled);

        /// <summary>
        /// Parses a SQL string and replaces [ ] delimiters with " " .
        /// </summary>
        /// <param name="str">The SQL string to be parsed.</param>
        /// <returns>A SQL string with HPDb compatible delimiters.</returns>
        public static string ConvertBracketIdentifiers(string str)
        {
            // save the raw characters
            char[] chars = str.ToCharArray();

            // replace all the valid brackets and string literals with whitespace equal to the length removed
            str = RemoveValidBrackets.Replace(str, (Match m) => new string(' ', m.Value.Length));

            // find the remaining bracket offsets
            MatchCollection mc = FindBrackets.Matches(str);

            // use the offsets in the temp str to replace the original
            foreach (Match m in mc)
            {
                chars[m.Index] = '"';
            }

            return new string(chars);
        }

        /*
         * The following section contains all the type mappings between HPDb's custom types and ADO.NET
         *  standard types.
         */ 

        private static readonly Dictionary<DbType, HPDbDbType> DbTypeMapping;
        private static readonly Dictionary<HPDbDbType, DbType> HPDbDbTypeMapping;

        static HPDbUtility()
        {
            //DbType to HPDbDbType
            DbTypeMapping = new Dictionary<DbType, HPDbDbType>(27);
            DbTypeMapping.Add(DbType.AnsiString, HPDbDbType.Varchar);
            DbTypeMapping.Add(DbType.AnsiStringFixedLength, HPDbDbType.Char);
            //DbTypeMapping.Add(DbType.Binary, HPDbDbType.Undefined);
            //DbTypeMapping.Add(DbType.Boolean, HPDbDbType.Undefined);
            //DbTypeMapping.Add(DbType.Byte, HPDbDbType.Undefined);
            //DbTypeMapping.Add(DbType.Currency, HPDbDbType.Undefined);
            DbTypeMapping.Add(DbType.Date, HPDbDbType.Date);
            DbTypeMapping.Add(DbType.DateTime, HPDbDbType.Timestamp);
            //DbTypeMapping.Add(DbType.DateTime2, HPDbDbType.Undefined);
            //DbTypeMapping.Add(DbType.DateTimeOffset, HPDbDbType.Undefined);
            DbTypeMapping.Add(DbType.Decimal, HPDbDbType.Decimal);
            DbTypeMapping.Add(DbType.Double, HPDbDbType.Double);
            //DbTypeMapping.Add(DbType.Guid, HPDbDbType.Undefined);
            DbTypeMapping.Add(DbType.Int16, HPDbDbType.SmallInt);
            DbTypeMapping.Add(DbType.Int32, HPDbDbType.Integer);
            DbTypeMapping.Add(DbType.Int64, HPDbDbType.LargeInt);
            //DbTypeMapping.Add(DbType.Object, HPDbDbType.Undefined);
            //DbTypeMapping.Add(DbType.SByte, HPDbDbType.Undefined);
            DbTypeMapping.Add(DbType.Single, HPDbDbType.Float);
            DbTypeMapping.Add(DbType.String, HPDbDbType.Varchar); // wrong
            DbTypeMapping.Add(DbType.StringFixedLength, HPDbDbType.Char); // wrong
            DbTypeMapping.Add(DbType.Time, HPDbDbType.Time);
            DbTypeMapping.Add(DbType.UInt16, HPDbDbType.SmallIntUnsigned);
            DbTypeMapping.Add(DbType.UInt32, HPDbDbType.IntegerUnsigned);
            //DbTypeMapping.Add(DbType.UInt64, HPDbDbType.Undefined);
            DbTypeMapping.Add(DbType.VarNumeric, HPDbDbType.Numeric);
            //DbTypeMapping.Add(DbType.Xml, HPDbDbType.Undefined);

            // HPDbDbType to DbType
            HPDbDbTypeMapping = new Dictionary<HPDbDbType,DbType>(100);
            HPDbDbTypeMapping.Add(HPDbDbType.Undefined, DbType.Object);

            HPDbDbTypeMapping.Add(HPDbDbType.Char, DbType.StringFixedLength); //wrong
            HPDbDbTypeMapping.Add(HPDbDbType.Date, DbType.Date);
            HPDbDbTypeMapping.Add(HPDbDbType.Decimal, DbType.Decimal);
            HPDbDbTypeMapping.Add(HPDbDbType.DecimalUnsigned, DbType.Decimal); // wrong
            HPDbDbTypeMapping.Add(HPDbDbType.Double, DbType.Double);
            HPDbDbTypeMapping.Add(HPDbDbType.Float, DbType.Single);
            HPDbDbTypeMapping.Add(HPDbDbType.Integer, DbType.Int32);
            HPDbDbTypeMapping.Add(HPDbDbType.IntegerUnsigned, DbType.UInt32);
            HPDbDbTypeMapping.Add(HPDbDbType.Interval, DbType.Object); // wrong
            HPDbDbTypeMapping.Add(HPDbDbType.LargeInt, DbType.Int64);
            HPDbDbTypeMapping.Add(HPDbDbType.Numeric, DbType.VarNumeric);
            HPDbDbTypeMapping.Add(HPDbDbType.NumericUnsigned, DbType.VarNumeric); // wrong
            HPDbDbTypeMapping.Add(HPDbDbType.Real, DbType.Single);
            HPDbDbTypeMapping.Add(HPDbDbType.SmallInt, DbType.Int16);
            HPDbDbTypeMapping.Add(HPDbDbType.SmallIntUnsigned, DbType.UInt16);
            HPDbDbTypeMapping.Add(HPDbDbType.Time, DbType.Time);
            HPDbDbTypeMapping.Add(HPDbDbType.Timestamp, DbType.DateTime);
            HPDbDbTypeMapping.Add(HPDbDbType.Varchar, DbType.String); //wrong
        }

        public static HPDbDbType MapDbType(DbType type)
        {
            return DbTypeMapping.ContainsKey(type) ? DbTypeMapping[type] : HPDbDbType.Undefined;
        }

        public static DbType MapHPDbDbType(HPDbDbType type)
        {
            return HPDbDbTypeMapping.ContainsKey(type) ? HPDbDbTypeMapping[type] : DbType.Object;
        }
    }
}