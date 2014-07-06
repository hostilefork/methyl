![Methyl logo](https://raw.github.com/hostilefork/methyl/master/methyl-logo.png)

# Methyl Permissions-Based Tree/Graph Library

(a.k.a. "The DOM for Brian's Pathological Application Framework (a.k.a. Benzene)")

## AN IMPORTANT NOTICE

This is not the "actual" Methyl implementation.  It's a stub that was invented when the methodology being used for object-relational management in a memory-mapped file was shown to exhibit undefined behavior.  Also, it is an attempt to reinvent the system based on a new [Proxy pattern design](http://codereview.stackexchange.com/questions/33713/proxy-facade-implementation-concept-in-c11-impedance-matching-db-with-classes).

As such, it's a bit of a mess; grafted from various pieces.  The interface is probably about as good as such a concept can get; but no one has come with the coup-de-grace to say it **won't** work.  For a while it was stubbed out using a tree structure based on Qt's XML QDomElement, until performance problems made me go ahead and do a quick rewrite as a standard library based data structure.  It's good enough for testing (and persisting with serialization)...but the goal is to bring it back to being based on a transaction-logged memory-mapped file implementation.

Whether it has a future or not is uncertain.  I'm just stuffing it on GitHub to make it easier to backup and clone from "The Cloud" as I patch other parts back into it, and get some issue tracking.  The main goal is to get it to the point of being able to resurrect demos such that others could reasonably build/test them.  But for now, don't bother trying to read or run it, unless you are me!  It's *incomplete*, *not properly commented*, and *lacks test suites*!

## DESCRIPTION

Methyl originated as an attempt to "rethink the Browser DOM" (or XML DOM) as a data structure, to offer nicer invariants.  It also persisted nodes in memory-mapped files, with no serialization or deserialization necessary to load large documents.

Coupled with the new ideas was how to use a methodology like C++ smart pointers to manage node lifetime.  Inserting into the tree could only be done with a `Tree`, and convey a "transfer of ownership" with move semantics (like a `unique_ptr` losing ownership).  By analogy, removing from the tree would give a `Tree` back...thus transferring ownership responsibility to the restructuring code.  The same arguments for why smart pointers offer important advantages over garbage collection apply here.

*(Quick summary for the uninitiated: There are two major advantages.  One is that smart pointers can manage resource control at precise moments instead of the less predictable moments when a garbage collector might kick in.  The other is that smart pointers can check invariants at compile time.  In this case, an important one is checking that your code can't try to insert a node which already has a parent at another location in the tree.)*

Yet another feature thrown into the mix was to include with a node handle a notion of read-only vs read-or-write privileges, and to implement that using C++'s idea of carrying the `const` attribute.  I've written about how I feel many usages of const that I've seen are relatively unambitious, compared to what the feature *could* be used to check at compile-time...given that it can carry a powerful transitive check through the call graph:

http://blog.hostilefork.com/transitive-power-of-cpp-const/

*(Note: Functional programming advocates might turn their noses up at the idea that such an oddly-shaped tool be applied to making architectures more formal.  "If you wanted that, why are you using C++?"  Perhaps.  But if we're going to be trading jabs I'll mention that the actual main reason Haskell code has no side effects because no one ever uses Haskell programs...  :-P  It was interesting to me to see how far one could get with the idea.)*

That's just a bit of the idea.  But bear in mind this iteration of the Methyl database is still at the thought experiment stage.  To learn more about the project, check out [methyl.hostilefork.com](http://methyl.hostilefork.com).

## LICENSE

License is under the GPLv3 for this initial source release.  Submodules have their own licenses, so read those to find out what they are.

I'm pretty sure that at this point the only person who can edit and comprehend the code for this is me.  So that's something I'm setting to turn around by taking my own advice on almost everything else: *"publish early, publish often"*.  Except this code has been churning and seething for a long time, so the "early" bit is out the window.

Hopefully it will be beaten into good enough shape for at least a demo.
