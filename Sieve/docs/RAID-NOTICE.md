# The infinite RAID: authorship, and a request

**The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia).**

The infinite RAID (IDEAS §19.150, §19.154, §19.176) is Sieve's system for keeping saves and data
across many drives, and in later stages across many machines, every piece checked by its SHA-256.
Its code is `core/include/sieve/raid.hpp`, `core/src/raid.cpp` and `core/src/raid_files.hpp`, with stage 3's
`core/include/sieve/seal.hpp`, `core/src/seal.cpp`, `core/include/sieve/raid_net.hpp`, `core/src/raid_net.cpp`
and `tools/cli/raid_socket.hpp`/`.cpp`, and the `sieve raid` command.

## The licence: GNU AGPL v3, with one additional term

The RAID is released under the GNU Affero General Public License v3, as all of Potentia is (LICENSE
at the repository's root), with one additional term permitted by its section 7(b):

> **Attribution.** Whoever conveys the RAID's files, or a work based on them, must keep in them the
> notice "The infinite RAID save and data preservation system was designed by Edward James Gordon
> (Potentia)", and must show that notice wherever the work shows its legal notices or its credits.

That term is stated at the top of each of the RAID's source files, as section 7 asks.

## A request (not a condition of the licence)

Whoever uses the infinite RAID in a system run for profit is asked to give a share of that profit to
charity, the share settled by open, democratic debate among those who use it, so that every business
that gains from it is held to the same measure (IDEAS §19.177).

This is a request, not a term of the licence: the AGPL allows no further restriction of that kind,
and anyone may remove one. It is written here so that it travels with the code and is seen.

## Why this licence (12 November 2026)

Edward asked for the RAID to be locked down under a licence of its own if that were possible, and
for his authorship of it to be credited in any case; "lawyers aside", the choice of best fit was left
to Claude. The RAID had already been published under the AGPL, and copies published under it stay
under it. The AGPL lets a licensor require that author attributions be preserved (section 7(b)), and
that requirement cannot be removed, so the credit is secured in the licence itself. A charity share
cannot be added to the AGPL as a binding term (sections 7 and 10 let anyone remove it), so it is
written as a request. A permissive licence would let others take the RAID closed; the AGPL keeps it
open and, with the term above, keeps its author's name on it. A licence with a binding charity share
would mean leaving the AGPL for this part: possible for new code by its author, but it should be
drafted by a lawyer, and it would no longer be open source as the Open Source Initiative defines it.
