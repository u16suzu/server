/* Copyright (c) 2021, 2022, MariaDB Corporation.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA */

#ifndef LEX_CHARSET_INCLUDED
#define LEX_CHARSET_INCLUDED

/*
  Parse time character set and collation.

  Can be:

  1. Empty (not specified on the column level):
     CREATE TABLE t1 (a CHAR(10)) CHARACTER SET latin2;        -- (1a)
     CREATE TABLE t1 (a CHAR(10));                             -- (1b)

  2. Precisely typed:
     CREATE TABLE t1 (a CHAR(10) COLLATE latin1_bin);          -- (2a)
     CREATE TABLE t1 (
       a CHAR(10) CHARACTER SET latin1 COLLATE latin1_bin);    -- (2b)

  3. Contextually typed:
     CREATE TABLE t2 (a CHAR(10) BINARY) CHARACTER SET latin2; -- (3a)
     CREATE TABLE t2 (a CHAR(10) BINARY);                      -- (3b)
     CREATE TABLE t2 (a CHAR(10) COLLATE DEFAULT)
       CHARACER SET latin2 COLLATE latin2_bin;                 -- (3c)

  In case of an empty or a contextually typed collation,
  it is a subject to later resolution, when the context
  character set becomes known in the end of the CREATE statement:
  - either after the explicit table level CHARACTER SET, like in (1a,3a,3c)
  - or by the inhereted database level CHARACTER SET, like in (1b,3b)

  Resolution happens in Type_handler::Column_definition_prepare_stage1().
*/
struct Lex_exact_charset_extended_collation_attrs_st
{
public:
  enum Type
  {
    TYPE_EMPTY= 0,
    TYPE_CHARACTER_SET= 1,
    TYPE_COLLATE_EXACT= 2,
    TYPE_COLLATE_CONTEXTUALLY_TYPED= 3
  };

// Number of bits required to store enum Type values

#define LEX_CHARSET_COLLATION_TYPE_BITS 2
  static_assert(((1<<LEX_CHARSET_COLLATION_TYPE_BITS)-1) >=
                 TYPE_COLLATE_CONTEXTUALLY_TYPED,
                 "Lex_exact_charset_extended_collation_attrs_st::Type bits");

protected:
  CHARSET_INFO *m_ci;
  Type m_type;
public:
  static CHARSET_INFO *find_bin_collation(CHARSET_INFO *cs);
  static CHARSET_INFO *find_default_collation(CHARSET_INFO *cs);
public:
  void init()
  {
    m_ci= NULL;
    m_type= TYPE_EMPTY;
  }
  void init(CHARSET_INFO *cs, Type type)
  {
    DBUG_ASSERT(cs || type == TYPE_EMPTY);
    m_ci= cs;
    m_type= type;
  }
  bool is_empty() const
  {
    return m_type == TYPE_EMPTY;
  }
  void set_charset(CHARSET_INFO *cs)
  {
    DBUG_ASSERT(cs);
    m_ci= cs;
    m_type= TYPE_CHARACTER_SET;
  }
  void set_charset_collate_default(CHARSET_INFO *cs)
  {
    DBUG_ASSERT(cs);
    m_ci= cs;
    m_type= TYPE_COLLATE_EXACT;
  }
  bool set_charset_collate_binary(CHARSET_INFO *cs)
  {
    DBUG_ASSERT(cs);
    if (!(cs= find_bin_collation(cs)))
      return true;
    m_ci= cs;
    m_type= TYPE_COLLATE_EXACT;
    return false;
  }
  bool set_charset_collate_exact(CHARSET_INFO *cs,
                                 CHARSET_INFO *cl);
  void set_collate_default()
  {
    m_ci= &my_collation_contextually_typed_default;
    m_type= TYPE_COLLATE_CONTEXTUALLY_TYPED;
  }
  void set_contextually_typed_binary_style()
  {
    m_ci= &my_collation_contextually_typed_binary;
    m_type= TYPE_COLLATE_CONTEXTUALLY_TYPED;
  }
  bool is_contextually_typed_collate_default() const
  {
    return m_ci == &my_collation_contextually_typed_default;
  }
  bool is_contextually_typed_binary_style() const
  {
    return m_ci == &my_collation_contextually_typed_binary;
  }
  void set_collate_exact(CHARSET_INFO *cl)
  {
    DBUG_ASSERT(cl);
    m_ci= cl;
    m_type= TYPE_COLLATE_EXACT;
  }
  CHARSET_INFO *charset_info() const
  {
    return m_ci;
  }
  Type type() const
  {
    return m_type;
  }
  bool is_contextually_typed_collation() const
  {
    return m_type == TYPE_COLLATE_CONTEXTUALLY_TYPED;
  }
  CHARSET_INFO *resolved_to_character_set(CHARSET_INFO *cs) const;
  bool merge_charset_clause_and_collate_clause(
                     const Lex_exact_charset_extended_collation_attrs_st &cl);
  bool merge_collate_clause_and_collate_clause(
                     const Lex_exact_charset_extended_collation_attrs_st &cl);
};


/*
  CHARACTER SET cs_exact [COLLATE cl_exact_or_context]
*/
class Lex_exact_charset_opt_extended_collate
{
  CHARSET_INFO *m_ci;
  bool m_with_collate;
public:
  Lex_exact_charset_opt_extended_collate(CHARSET_INFO *ci, bool with_collate)
   :m_ci(ci), m_with_collate(with_collate)
  {
    DBUG_ASSERT(m_ci);
    // Item_func_set_collation uses non-default collations in "ci"
    //DBUG_ASSERT(m_ci->default_flag() || m_with_collate);
  }
  /*
    Add another COLLATE clause (exact or context).
    So the full syntax looks like:
      CHARACTER SET cs [COLLATE cl] ... COLLATE cl2
  */
  bool merge_collate_or_error(
                     const Lex_exact_charset_extended_collation_attrs_st &cl);
  bool merge_opt_collate_or_error(
                     const Lex_exact_charset_extended_collation_attrs_st &cl)
  {
    if (cl.is_empty())
      return false;
    return merge_collate_or_error(cl);
  }
  CHARSET_INFO *charset_info() const { return m_ci; }
  bool with_collate() const { return m_with_collate; }
};


class Lex_exact_charset_extended_collation_attrs:
                        public Lex_exact_charset_extended_collation_attrs_st
{
public:
  Lex_exact_charset_extended_collation_attrs()
  {
    init();
  }
  Lex_exact_charset_extended_collation_attrs(CHARSET_INFO *collation, Type type)
  {
    init(collation, type);
  }
  static Lex_exact_charset_extended_collation_attrs national(bool bin_mod)
  {
    return bin_mod ?
      Lex_exact_charset_extended_collation_attrs(&my_charset_utf8mb3_bin,
                                                 TYPE_COLLATE_EXACT) :
      Lex_exact_charset_extended_collation_attrs(&my_charset_utf8mb3_general_ci,
                                                 TYPE_CHARACTER_SET);
  }
};


using Lex_column_charset_collation_attrs_st =
        Lex_exact_charset_extended_collation_attrs_st;

using Lex_column_charset_collation_attrs =
        Lex_exact_charset_extended_collation_attrs;


#endif // LEX_CHARSET_INCLUDED
