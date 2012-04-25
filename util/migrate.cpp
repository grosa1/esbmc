#include <stdint.h>

#include "migrate.h"
#include "prefix.h"

#include <config.h>
#include <simplify_expr.h>

// File for old irep -> new irep conversions.


void
real_migrate_type(const typet &type, type2tc &new_type_ref)
{

  if (type.id() == "bool") {
    bool_type2t *b = new bool_type2t();
    new_type_ref = type2tc(b);
  } else if (type.id() == "signedbv") {
    irep_idt width = type.width();
    unsigned int iwidth = strtol(width.as_string().c_str(), NULL, 10);
    signedbv_type2t *s = new signedbv_type2t(iwidth);
    new_type_ref = type2tc(s);
  } else if (type.id() == "unsignedbv") {
    irep_idt width = type.width();
    unsigned int iwidth = strtol(width.as_string().c_str(), NULL, 10);
    unsignedbv_type2t *s = new unsignedbv_type2t(iwidth);
    new_type_ref = type2tc(s);
  } else if (type.id() == "c_enum" || type.id() == "incomplete_c_enum") {
    // 6.7.2.2.3 of C99 says enumeration values shall have "int" types.
    signedbv_type2t *s = new signedbv_type2t(config.ansi_c.int_width);
    new_type_ref = type2tc(s);
  } else if (type.id() == "array") {
    type2tc subtype;
    expr2tc size((expr2t *)NULL);
    bool is_infinite = false;

    migrate_type(type.subtype(), subtype);

    if (type.find("size").id() == "infinity") {
      is_infinite = true;
    } else {
      exprt sz = (exprt&)type.find("size");
      simplify(sz);
      migrate_expr(sz, size);
    }

    array_type2t *a = new array_type2t(subtype, size, is_infinite);
    new_type_ref = type2tc(a);
  } else if (type.id() == "pointer") {
    type2tc subtype;

    migrate_type(type.subtype(), subtype);

    pointer_type2t *p = new pointer_type2t(subtype);
    new_type_ref = type2tc(p);
  } else if (type.id() == "empty") {
    empty_type2t *e = new empty_type2t();
    new_type_ref = type2tc(e);
  } else if (type.id() == "symbol") {
    symbol_type2t *s = new symbol_type2t(type.identifier());
    new_type_ref = type2tc(s);
  } else if (type.id() == "struct") {
    std::vector<type2tc> members;
    std::vector<std::string> names;
    struct_typet &strct = (struct_typet&)type;
    struct_union_typet::componentst comps = strct.components();

    for (struct_union_typet::componentst::const_iterator it = comps.begin();
         it != comps.end(); it++) {
      type2tc ref;
      migrate_type((const typet&)it->type(), ref);

      members.push_back(ref);
      names.push_back(it->get("name").as_string());
    }

    std::string name = type.get_string("tag");
    assert(name != "");
    struct_type2t *s = new struct_type2t(members, names, name);
    new_type_ref = type2tc(s);
  } else if (type.id() == "union") {
    std::vector<type2tc> members;
    std::vector<std::string> names;
    union_typet &strct = (union_typet&)type;
    struct_union_typet::componentst comps = strct.components();

    for (struct_union_typet::componentst::const_iterator it = comps.begin();
         it != comps.end(); it++) {
      type2tc ref;
      migrate_type((const typet&)it->type(), ref);

      members.push_back(ref);
      names.push_back(it->get("name").as_string());
    }

    std::string name = type.get_string("tag");
    assert(name != "");
    union_type2t *u = new union_type2t(members, names, name);
    new_type_ref = type2tc(u);
  } else if (type.id() == "fixedbv") {
    std::string fract = type.get_string("width");
    assert(fract != "");
    unsigned int frac_bits = strtol(fract.c_str(), NULL, 10);

    std::string ints = type.get_string("integer_bits");
    assert(ints != "");
    unsigned int int_bits = strtol(ints.c_str(), NULL, 10);

    fixedbv_type2t *f = new fixedbv_type2t(frac_bits, int_bits);
    new_type_ref = type2tc(f);
  } else if (type.id() == "code") {
    code_type2t *c = new code_type2t();
    new_type_ref = type2tc(c);
  } else {
    type.dump();
    assert(0);
  }
}

void
migrate_type(const typet &type, type2tc &new_type_ref)
{

  if (type.id() == "bool") {
    new_type_ref = type_pool.get_bool();
  } else if (type.id() == "signedbv") {
    new_type_ref = type_pool.get_signedbv(type);
  } else if (type.id() == "unsignedbv") {
    new_type_ref = type_pool.get_unsignedbv(type);
  } else if (type.id() == "c_enum" || type.id() == "incomplete_c_enum") {
    // 6.7.2.2.3 of C99 says enumeration values shall have "int" types.
    new_type_ref = type_pool.get_int(config.ansi_c.int_width);
  } else if (type.id() == "array") {
    new_type_ref = type_pool.get_array(type);
  } else if (type.id() == "pointer") {
    new_type_ref = type_pool.get_pointer(type);
  } else if (type.id() == "empty") {
    new_type_ref = type_pool.get_empty();
  } else if (type.id() == "symbol") {
    new_type_ref = type_pool.get_symbol(type);
  } else if (type.id() == "struct") {
    new_type_ref = type_pool.get_struct(type);
  } else if (type.id() == "union") {
    new_type_ref = type_pool.get_union(type);
  } else if (type.id() == "fixedbv") {
    new_type_ref = type_pool.get_fixedbv(type);
  } else if (type.id() == "code") {
    new_type_ref = type_pool.get_code();
  } else {
    type.dump();
    assert(0);
  }
}

static void
splice_expr(const exprt &expr, expr2tc &new_expr_ref)
{
  exprt expr_twopart = expr;
  exprt expr_recurse = expr;

  exprt popped = expr_recurse.operands()[expr_recurse.operands().size()-1];
  expr_recurse.operands().pop_back();

  expr_twopart.operands().clear();
  expr_twopart.copy_to_operands(expr_recurse, popped);

  migrate_expr(expr_twopart, new_expr_ref);
}

static void
convert_operand_pair(const exprt expr, expr2tc &arg1, expr2tc &arg2)
{

  migrate_expr(expr.op0(), arg1);
  migrate_expr(expr.op1(), arg2);
}

void
migrate_expr(const exprt &expr, expr2tc &new_expr_ref)
{
  type2tc type;

  if (expr.id() == "symbol") {
    migrate_type(expr.type(), type);
    expr2t *new_expr = new symbol2t(type, expr.identifier().as_string());
    new_expr_ref = expr2tc(new_expr);
  } else if (expr.id() == "nondet_symbol") {
    migrate_type(expr.type(), type);
    expr2t *new_expr = new symbol2t(type,
                                    "nondet$" + expr.identifier().as_string());
    new_expr_ref = expr2tc(new_expr);
  } else if (expr.id() == "constant" && expr.type().id() != "pointer" &&
             expr.type().id() != "bool" && expr.type().id() != "c_enum" &&
             expr.type().id() != "fixedbv" && expr.type().id() != "array") {
    migrate_type(expr.type(), type);

    bool is_signed = false;
    if (type->type_id == type2t::signedbv_id)
      is_signed = true;

    mp_integer val = binary2integer(expr.value().as_string(), is_signed);

    expr2t *new_expr = new constant_int2t(type, val);
    new_expr_ref = expr2tc(new_expr);
  } else if (expr.id() == "constant" && expr.type().id() == "c_enum") {
    migrate_type(expr.type(), type);

    uint64_t enumval = atoi(expr.value().as_string().c_str());

    expr2t *new_expr = new constant_int2t(type, BigInt(enumval));
    new_expr_ref = expr2tc(new_expr);
  } else if (expr.id() == "constant" && expr.type().id() == "bool") {
    std::string theval = expr.value().as_string();
    if (theval == "true")
      new_expr_ref = expr2tc(new constant_bool2t(true));
    else
      new_expr_ref = expr2tc(new constant_bool2t(false));
  } else if (expr.id() == "constant" && expr.type().id() == "pointer" &&
             expr.value() == "NULL") {
    // Null is a symbol with pointer type.
     migrate_type(expr.type(), type);

    expr2t *new_expr = new symbol2t(type, std::string("NULL"));
    new_expr_ref = expr2tc(new_expr);
  } else if (expr.id() == "constant" && expr.type().id() == "fixedbv") {
    migrate_type(expr.type(), type);

    fixedbvt bv(expr);

    expr2t *new_expr = new constant_fixedbv2t(type, bv);
    new_expr_ref = expr2tc(new_expr);
  } else if (expr.id() == "typecast") {
    assert(expr.op0().id_string() != "");
    expr2tc old_expr;

    migrate_type(expr.type(), type);

    migrate_expr(expr.op0(), old_expr);

    typecast2t *t = new typecast2t(type, old_expr);
    new_expr_ref = expr2tc(t);
  } else if (expr.id() == "struct") {
    migrate_type(expr.type(), type);

    std::vector<expr2tc> members;
    forall_operands(it, expr) {
      expr2tc new_ref;
      migrate_expr(*it, new_ref);

      members.push_back(new_ref);
    }

    constant_struct2t *s = new constant_struct2t(type, members);
    new_expr_ref = expr2tc(s);
  } else if (expr.id() == "union") {
    migrate_type(expr.type(), type);

    std::vector<expr2tc> members;
    forall_operands(it, expr) {
      expr2tc new_ref;
      migrate_expr(*it, new_ref);

      members.push_back(new_ref);
    }

    constant_union2t *u = new constant_union2t(type, members);
    new_expr_ref = expr2tc(u);
  } else if (expr.id() == "string-constant") {
    std::string thestring = expr.value().as_string();
    typet thetype = expr.type();
    assert(thetype.add("size").id() == "constant");
    exprt &face = (exprt&)thetype.add("size");
    std::string thelen = face.value().as_string();
    mp_integer val = binary2integer(thelen, false);

    type2tc t = type2tc(new string_type2t(val.to_long()));

    new_expr_ref = expr2tc(new constant_string2t(t, thestring));
  } else if (expr.id() == "constant" && expr.type().id() == "array") {
    // Fixed size array.
    migrate_type(expr.type(), type);

    std::vector<expr2tc> members;
    forall_operands(it, expr) {
      expr2tc new_ref;
      migrate_expr(*it, new_ref);

      members.push_back(new_ref);
    }

    constant_array2t *a = new constant_array2t(type, members);
    new_expr_ref = expr2tc(a);
  } else if (expr.id() == "array_of") {
    migrate_type(expr.type(), type);

    assert(expr.operands().size() == 1);
    expr2tc new_value;
    migrate_expr(expr.op0(), new_value);

    constant_array_of2t *a = new constant_array_of2t(type, new_value);
    new_expr_ref = expr2tc(a);
  } else if (expr.id() == "if") {
    migrate_type(expr.type(), type);

    expr2tc cond, true_val, false_val;
    migrate_expr(expr.op0(), cond);
    migrate_expr(expr.op1(), true_val);
    migrate_expr(expr.op2(), false_val);

    if2t *i = new if2t(type, cond, true_val, false_val);
    new_expr_ref = expr2tc(i);
  } else if (expr.id() == "=") {
    expr2tc side1, side2;

    convert_operand_pair(expr, side1, side2);

    equality2t *e = new equality2t(side1, side2);
    new_expr_ref = expr2tc(e);
  } else if (expr.id() == "!=" || expr.id() == "notequal") {
    expr2tc side1, side2;

    convert_operand_pair(expr, side1, side2);

    notequal2t *n = new notequal2t(side1, side2);
    new_expr_ref = expr2tc(n);
   } else if (expr.id() == "<") {
    expr2tc side1, side2;

    convert_operand_pair(expr, side1, side2);

    lessthan2t *n = new lessthan2t(side1, side2);
    new_expr_ref = expr2tc(n);
   } else if (expr.id() == ">") {
    expr2tc side1, side2;
    migrate_expr(expr.op0(), side1);
    migrate_expr(expr.op1(), side2);

    greaterthan2t *n = new greaterthan2t(side1, side2);
    new_expr_ref = expr2tc(n);
  } else if (expr.id() == "<=") {
    expr2tc side1, side2;

    convert_operand_pair(expr, side1, side2);

    lessthanequal2t *n = new lessthanequal2t(side1, side2);
    new_expr_ref = expr2tc(n);
  } else if (expr.id() == ">=") {
    expr2tc side1, side2;

    convert_operand_pair(expr, side1, side2);

    greaterthanequal2t *n = new greaterthanequal2t(side1, side2);
    new_expr_ref = expr2tc(n);
  } else if (expr.id() == "not") {
    assert(expr.type().id() == "bool");
    expr2tc theval;
    migrate_expr(expr.op0(), theval);

    not2t *n = new not2t(theval);
    new_expr_ref = expr2tc(n);
  } else if (expr.id() == "and") {
    assert(expr.type().id() == "bool");
    expr2tc side1, side2;
    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    and2t *a = new and2t(side1, side2);
    new_expr_ref = expr2tc(a);
  } else if (expr.id() == "or") {
    assert(expr.type().id() == "bool");
    expr2tc side1, side2;

    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    or2t *o = new or2t(side1, side2);
    new_expr_ref = expr2tc(o);
  } else if (expr.id() == "xor") {
    assert(expr.type().id() == "bool");
    assert(expr.operands().size() == 2);
    expr2tc side1, side2;

    convert_operand_pair(expr, side1, side2);

    xor2t *x = new xor2t(side1, side2);
    new_expr_ref = expr2tc(x);
  } else if (expr.id() == "bitand") {
    migrate_type(expr.type(), type);

    expr2tc side1, side2;
    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    bitand2t *a = new bitand2t(type, side1, side2);
    new_expr_ref = expr2tc(a);
  } else if (expr.id() == "bitor") {
    migrate_type(expr.type(), type);

    expr2tc side1, side2;
    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    bitor2t *o = new bitor2t(type, side1, side2);
    new_expr_ref = expr2tc(o);
  } else if (expr.id() == "bitxor") {
    migrate_type(expr.type(), type);

    expr2tc side1, side2;
    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    bitxor2t *x = new bitxor2t(type, side1, side2);
    new_expr_ref = expr2tc(x);
  } else if (expr.id() == "bitnand") {
    migrate_type(expr.type(), type);

    expr2tc side1, side2;
    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    bitnand2t *n = new bitnand2t(type, side1, side2);
    new_expr_ref = expr2tc(n);
  } else if (expr.id() == "bitnor") {
    migrate_type(expr.type(), type);

    expr2tc side1, side2;
    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    bitnor2t *o = new bitnor2t(type, side1, side2);
    new_expr_ref = expr2tc(o);
  } else if (expr.id() == "bitnxor") {
    migrate_type(expr.type(), type);

    expr2tc side1, side2;
    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    bitnxor2t *x = new bitnxor2t(type, side1, side2);
    new_expr_ref = expr2tc(x);
  } else if (expr.id() == "lshr") {
    migrate_type(expr.type(), type);

    expr2tc side1, side2;
    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    lshr2t *s = new lshr2t(type, side1, side2);
    new_expr_ref = expr2tc(s);
  } else if (expr.id() == "unary-") {
    migrate_type(expr.type(), type);

    expr2tc theval;
    migrate_expr(expr.op0(), theval);

    neg2t *n = new neg2t(type, theval);
    new_expr_ref = expr2tc(n);
  } else if (expr.id() == "abs") {
    migrate_type(expr.type(), type);

    expr2tc theval;
    migrate_expr(expr.op0(), theval);

    abs2t *a = new abs2t(type, theval);
    new_expr_ref = expr2tc(a);
  } else if (expr.id() == "+") {
    migrate_type(expr.type(), type);

    expr2tc side1, side2;
    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    convert_operand_pair(expr, side1, side2);

    add2t *a = new add2t(type, side1, side2);
    new_expr_ref = expr2tc(a);
  } else if (expr.id() == "-") {
    migrate_type(expr.type(), type);

    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    expr2tc side1, side2;
    convert_operand_pair(expr, side1, side2);

    sub2t *s = new sub2t(type, side1, side2);
    new_expr_ref = expr2tc(s);
  } else if (expr.id() == "*") {
    migrate_type(expr.type(), type);

    if (expr.operands().size() > 2) {
      splice_expr(expr, new_expr_ref);
      return;
    }

    expr2tc side1, side2;
    convert_operand_pair(expr, side1, side2);

    mul2t *s = new mul2t(type, side1, side2);
    new_expr_ref = expr2tc(s);
  } else if (expr.id() == "/") {
    migrate_type(expr.type(), type);

    assert(expr.operands().size() == 2);

    expr2tc side1, side2;
    convert_operand_pair(expr, side1, side2);

    div2t *d = new div2t(type, side1, side2);
    new_expr_ref = expr2tc(d);
  } else if (expr.id() == "mod") {
    migrate_type(expr.type(), type);

    assert(expr.operands().size() == 2);

    expr2tc side1, side2;
    convert_operand_pair(expr, side1, side2);

    modulus2t *m = new modulus2t(type, side1, side2);
    new_expr_ref = expr2tc(m);
  } else if (expr.id() == "shl") {
    migrate_type(expr.type(), type);

    assert(expr.operands().size() == 2);

    expr2tc side1, side2;
    convert_operand_pair(expr, side1, side2);

    shl2t *s = new shl2t(type, side1, side2);
    new_expr_ref = expr2tc(s);
  } else if (expr.id() == "ashr") {
    migrate_type(expr.type(), type);

    assert(expr.operands().size() == 2);

    expr2tc side1, side2;
    convert_operand_pair(expr, side1, side2);

    ashr2t *a = new ashr2t(type, side1, side2);
    new_expr_ref = expr2tc(a);
  } else if (expr.id() == "pointer_offset") {
    migrate_type(expr.type(), type);

    expr2tc theval;
    migrate_expr(expr.op0(), theval);

    pointer_offset2t *p = new pointer_offset2t(type, theval);
    new_expr_ref = expr2tc(p);
  } else if (expr.id() == "pointer_object") {
    migrate_type(expr.type(), type);

    expr2tc theval;
    migrate_expr(expr.op0(), theval);

    pointer_object2t *p = new pointer_object2t(type, theval);
    new_expr_ref = expr2tc(p);
  } else if (expr.id() == "address_of") {
    assert(expr.type().id() == "pointer");

    migrate_type(expr.type().subtype(), type);

    expr2tc theval;
    migrate_expr(expr.op0(), theval);

    address_of2t *a = new address_of2t(type, theval);
    new_expr_ref = expr2tc(a);
   } else if (expr.id() == "byte_extract_little_endian" ||
             expr.id() == "byte_extract_big_endian") {
    migrate_type(expr.type(), type);

    assert(expr.operands().size() == 2);

    expr2tc side1, side2;
    convert_operand_pair(expr, side1, side2);

    bool big_endian = (expr.id() == "byte_extract_big_endian") ? true : false;

    byte_extract2t *b = new byte_extract2t(type, big_endian, side1, side2);
    new_expr_ref = expr2tc(b);
  } else if (expr.id() == "byte_update_little_endian" ||
             expr.id() == "byte_update_big_endian") {
    migrate_type(expr.type(), type);

    assert(expr.operands().size() == 3);

    expr2tc sourceval, offs;
    convert_operand_pair(expr, sourceval, offs);

    expr2tc update;
    migrate_expr(expr.op2(), update);

    bool big_endian = (expr.id() == "byte_update_big_endian") ? true : false;

    byte_update2t *u = new byte_update2t(type, big_endian,
                                         sourceval, offs, update);
    new_expr_ref = expr2tc(u);
  } else if (expr.id() == "with") {
    migrate_type(expr.type(), type);

    expr2tc sourcedata, idx;
    migrate_expr(expr.op0(), sourcedata);

    if (expr.op1().id() == "member_name") {
      idx = expr2tc(new constant_string2t(type2tc(new string_type2t(1)),
                                    expr.op1().get_string("component_name")));
    } else {
      migrate_expr(expr.op1(), idx);
    }

    expr2tc update;
    migrate_expr(expr.op2(), update);

    with2t *w = new with2t(type, sourcedata, idx, update);
    new_expr_ref = expr2tc(w);
  } else if (expr.id() == "member") {
    migrate_type(expr.type(), type);

    expr2tc sourcedata;
    migrate_expr(expr.op0(), sourcedata);

    constant_string2t idx(type2tc(new string_type2t(1)),
                          expr.get_string("component_name"));

    member2t *m = new member2t(type, sourcedata, idx);
    new_expr_ref = expr2tc(m);
  } else if (expr.id() == "index") {
    migrate_type(expr.type(), type);

    assert(expr.operands().size() == 2);
    expr2tc source, index;
    convert_operand_pair(expr, source, index);

    index2t *i = new index2t(type, source, index);
    new_expr_ref = expr2tc(i);
  } else if (expr.id() == "memory-leak") {
    // Memory leaks are in fact selects/indexes.
    migrate_type(expr.type(), type);

    assert(expr.operands().size() == 2);
    assert(expr.type().id() == "bool");
    expr2tc source, index;
    convert_operand_pair(expr, source, index);

    index2t *i = new index2t(type, source, index);
    new_expr_ref = expr2tc(i);
  } else if (expr.id() == "zero_string") {
    assert(expr.operands().size() == 1);

    expr2tc string;
    migrate_expr(expr.op0(), string);

    zero_string2t *s = new zero_string2t(string);
    new_expr_ref = expr2tc(s);
  } else if (expr.id() == "zero_string_length") {
    assert(expr.operands().size() == 1);

    expr2tc string;
    migrate_expr(expr.op0(), string);

    zero_length_string2t *s = new zero_length_string2t(string);
    new_expr_ref = expr2tc(s);
  } else if (expr.id() == "isnan") {
    assert(expr.operands().size() == 1);

    expr2tc val;
    migrate_expr(expr.op0(), val);

    isnan2t *i = new isnan2t(val);
    new_expr_ref = expr2tc(i);
  } else if (expr.id() == "width") {
    assert(expr.operands().size() == 1);
    migrate_type(expr.type(), type);

    uint64_t thewidth = type->get_width();
    type2tc inttype(new unsignedbv_type2t(config.ansi_c.int_width));
    new_expr_ref = expr2tc(new constant_int2t(inttype, BigInt(thewidth)));
  } else if (expr.id() == "same-object") {
    assert(expr.operands().size() == 2);
    assert(expr.type().id() == "bool");
    expr2tc op0, op1;
    convert_operand_pair(expr, op0, op1);

    same_object2t *s = new same_object2t(op0, op1);
    new_expr_ref = expr2tc(s);
  } else if (expr.id() == "invalid-object") {
    assert(expr.type().id() == "pointer");
    type2tc pointertype(new pointer_type2t(type2tc(new empty_type2t())));
    new_expr_ref = expr2tc(new symbol2t(pointertype, "INVALID"));
  } else if (expr.id() == "unary+") {
    migrate_expr(expr.op0(), new_expr_ref);
  } else if (expr.id() == "overflow-+") {
    assert(expr.type().id() == "bool");
    expr2tc op0, op1;
    convert_operand_pair(expr, op0, op1);
    expr2tc add = expr2tc(new add2t(op0->type, op0, op1)); // XXX type?
    new_expr_ref = expr2tc(new overflow2t(add));
  } else if (expr.id() == "overflow--") {
    assert(expr.type().id() == "bool");
    expr2tc op0, op1;
    convert_operand_pair(expr, op0, op1);
    expr2tc sub = expr2tc(new sub2t(op0->type, op0, op1)); // XXX type?
    new_expr_ref = expr2tc(new overflow2t(sub));
  } else if (expr.id() == "overflow-*") {
    assert(expr.type().id() == "bool");
    expr2tc op0, op1;
    convert_operand_pair(expr, op0, op1);
    expr2tc mul = expr2tc(new mul2t(op0->type, op0, op1)); // XXX type?
    new_expr_ref = expr2tc(new overflow2t(mul));
  } else if (has_prefix(expr.id_string(), "overflow-typecast-")) {
    unsigned bits = atoi(expr.id_string().c_str() + 18);
    expr2tc operand;
    migrate_expr(expr.op0(), operand);
    new_expr_ref = expr2tc(new overflow_cast2t(operand, bits));
  } else if (expr.id() == "overflow-unary-") {
    assert(expr.type().id() == "bool");
    expr2tc operand;
    migrate_expr(expr.op0(), operand);
    new_expr_ref = expr2tc(new overflow_neg2t(operand));
  } else {
    expr.dump();
    throw new std::string("migrate expr failed");
  }
}
