# Principles of Extensible C++ Library Design

I set out to extend the JSONPath module of **jsoncons** to follow JSON References (`$ref`-style pointers) during identifier selection. The feature itself amounted to fewer than 30 lines: detect a reference, resolve it, continue traversal. But the selectors were all `final`, construction was hard-wired, and reference resolution was embedded. Result: to add those <30 lines I had to fork and carry a codebase of well over 100K lines. This disproportion—tiny behavior change, massive surface to patch—is the quintessential example of why deliberate extensibility matters.

Library authors face a fundamental tension: they must provide stable, robust interfaces while enabling users to extend functionality for unforeseen use cases. When you freeze architecture through pervasive `final` classes, non-virtual construction, or rigid factories, you convert every unconventional user need into a fork. Fork pressure is a symptom of insufficient extensibility, not "power users being demanding."

This document explores three key principles for building C++ libraries that achieve this balance:

1. **Factory Pattern for Injection** - How to allow users to substitute custom implementations
2. **Small Function Decomposition** - Breaking down complex operations to maximize reusability
3. **Access Control Philosophy** - Balancing encapsulation with power user needs

**Target Audience:** C++ library authors and maintainers who want to design APIs that are both stable and extensible.

**Prerequisites:** Intermediate C++ knowledge including virtual functions, templates, and smart pointers.

---

## 1. Factory Pattern: Enabling Custom Class Injection

The Factory Pattern is fundamental to extensible design. It decouples object creation from usage, allowing users to inject custom implementations without modifying library code. C++ offers two primary approaches: classical runtime polymorphism and compile-time traits.

### 1.1 Classical C++: Virtual Factory with Runtime Registration

This approach uses virtual interfaces and a registration mechanism for maximum runtime flexibility.

**Step 1: Define the abstract interface**

```cpp
class IDataProcessor {
public:
    virtual ~IDataProcessor() = default;
    virtual void process(const Data& input, Result& output) = 0;
    virtual std::string name() const = 0;
};
```

**Step 2: Create a factory with registration**

```cpp
class ProcessorFactory {
public:
    using CreateFn = std::unique_ptr<IDataProcessor>(*)();
    
    static ProcessorFactory& instance() {
        static ProcessorFactory factory;
        return factory;
    }
    
    void register_processor(const std::string& type, CreateFn creator) {
        creators_[type] = creator;
    }
    
    std::unique_ptr<IDataProcessor> create(const std::string& type) {
        auto it = creators_.find(type);
        if (it == creators_.end()) {
            throw std::runtime_error("Unknown processor type: " + type);
        }
        return (it->second)();
    }
    
    std::vector<std::string> registered_types() const {
        std::vector<std::string> types;
        for (const auto& [key, _] : creators_) {
            types.push_back(key);
        }
        return types;
    }
    
private:
    std::unordered_map<std::string, CreateFn> creators_;
};
```

**Step 3: Library provides default implementations**

```cpp
class DefaultProcessor : public IDataProcessor {
public:
    void process(const Data& input, Result& output) override {
        // Standard processing logic
    }
    std::string name() const override { return "default"; }
};

// Auto-register during static initialization
namespace {
    bool default_registered = []() {
        ProcessorFactory::instance().register_processor("default",
            []() { return std::make_unique<DefaultProcessor>(); });
        return true;
    }();
}
```

**Step 4: Users inject custom implementations**

```cpp
// User code - no library modification needed
class LoggingProcessor : public IDataProcessor {
public:
    LoggingProcessor(std::shared_ptr<IDataProcessor> inner)
        : inner_(std::move(inner)) {}
        
    void process(const Data& input, Result& output) override {
        std::cout << "Processing: " << input.id() << "\n";
        inner_->process(input, output);
        std::cout << "Completed: " << output.status() << "\n";
    }
    
    std::string name() const override { return "logging_wrapper"; }
    
private:
    std::shared_ptr<IDataProcessor> inner_;
};

// Register the custom processor
ProcessorFactory::instance().register_processor("logging",
    []() { 
        auto inner = ProcessorFactory::instance().create("default");
        return std::make_unique<LoggingProcessor>(std::move(inner));
    });
```

**Advantages:**
- Runtime flexibility (load plugins, configure at runtime)
- Clean separation between interface and implementation
- Users extend without touching library source

**Trade-offs:**
- Virtual call overhead (usually negligible)
- Requires heap allocation for polymorphic objects
- Additional complexity for simple cases

### 1.2 Template-Based Factory: Traits and Policies

For zero-overhead abstraction, use compile-time policy injection via templates. This approach provides the same extensibility without runtime costs.

**Step 1: Define default policy with required interface**

```cpp
// Default policy - user policies must provide the same interface
struct DefaultProcessingPolicy {
    template<typename Data, typename Result>
    void process(const Data& input, Result& output) const {
        // Default implementation
        output.set_value(input.get_value() * 2);
    }
    
    static constexpr const char* name() { return "default"; }
};
```

**Step 2: Create templated class using the policy**

```cpp
template<typename ProcessingPolicy = DefaultProcessingPolicy>
class DataEngine {
public:
    explicit DataEngine(ProcessingPolicy policy = ProcessingPolicy())
        : policy_(std::move(policy)) {}
    
    template<typename Data, typename Result>
    void execute(const Data& input, Result& output) {
        validate_input(input);
        policy_.process(input, output);
        finalize_result(output);
    }
    
    const char* policy_name() const { return ProcessingPolicy::name(); }
    
private:
    ProcessingPolicy policy_;
    
    template<typename Data>
    void validate_input(const Data& input) {
        // Common validation logic
    }
    
    template<typename Result>
    void finalize_result(Result& output) {
        // Common finalization logic
    }
};
```

**Step 3: Define traits for type-based customization**

```cpp
// Traits pattern - specialize for custom types
template<typename T>
struct ProcessingTraits {
    using Policy = DefaultProcessingPolicy;
};

// User can specialize for their types
template<>
struct ProcessingTraits<MyCustomData> {
    using Policy = MyCustomProcessingPolicy;
};

// Factory function using traits
template<typename Data>
auto make_engine_for(const Data&) {
    using Policy = typename ProcessingTraits<Data>::Policy;
    return DataEngine<Policy>();
}
```

**Step 4: Users define custom policies**

```cpp
// User's custom policy - same interface, different behavior
struct CachingPolicy {
    mutable std::unordered_map<int, double> cache_;
    
    template<typename Data, typename Result>
    void process(const Data& input, Result& output) const {
        auto key = input.get_key();
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            output.set_value(it->second);
            return;
        }
        
        double value = expensive_computation(input);
        cache_[key] = value;
        output.set_value(value);
    }
    
    static constexpr const char* name() { return "caching"; }
    
private:
    double expensive_computation(const auto& input) const {
        // Complex calculation
        return input.get_value() * 3.14159;
    }
};

// Use it
DataEngine<CachingPolicy> engine;
engine.execute(data, result);
```

**Advanced: Policy combination**

```cpp
// Combine multiple policies via composition
template<typename PolicyA, typename PolicyB>
struct CombinedPolicy {
    PolicyA policy_a;
    PolicyB policy_b;
    
    template<typename Data, typename Result>
    void process(const Data& input, Result& output) const {
        Result temp;
        policy_a.process(input, temp);
        policy_b.process(temp, output);
    }
    
    static constexpr const char* name() { return "combined"; }
};

using MyEngine = DataEngine<CombinedPolicy<ValidationPolicy, CachingPolicy>>;
```

**Advantages:**
- Zero runtime overhead (everything inlined)
- Type-safe at compile time
- Optimal for performance-critical code

**Trade-offs:**
- Code bloat if many instantiations exist
- Compile-time selection only (no runtime plugin loading)
- Template error messages can be complex

### 1.3 Hybrid Approach: Type Erasure

When you need runtime flexibility without exposing virtual in the public API:

```cpp
class DataProcessor {
public:
    // Accept any callable with the right signature
    template<typename Fn>
    DataProcessor(Fn fn) 
        : impl_(std::make_shared<Model<Fn>>(std::move(fn))) {}
    
    void process(const Data& input, Result& output) const {
        impl_->process(input, output);
    }
    
private:
    struct Concept {
        virtual ~Concept() = default;
        virtual void process(const Data&, Result&) const = 0;
    };
    
    template<typename Fn>
    struct Model : Concept {
        Fn fn_;
        Model(Fn fn) : fn_(std::move(fn)) {}
        void process(const Data& input, Result& output) const override {
            fn_(input, output);
        }
    };
    
    std::shared_ptr<const Concept> impl_;
};

// Users can pass lambdas, function pointers, functors
DataProcessor proc([cache = std::map<int, double>{}](const Data& input, Result& output) mutable {
    // Custom logic with state
});
```

---

## 2. Small Function Decomposition: Clean Code for Reusability

"Clean Code" principles emphasize small, focused functions. For library design, this isn't just about readability—it directly impacts extensibility. When complex operations are broken into small, well-named functions, users can override or reuse specific steps without copying entire algorithms.

### 2.1 The Problem: Monolithic Methods

Consider a typical parsing class with a large monolithic method:

```cpp
class Parser {
public:
    Result parse(const std::string& input) {
        Result result;
        
        // 50 lines: validation, normalization, tokenization
        if (input.empty()) throw std::runtime_error("empty");
        std::string normalized = input;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        // ... more normalization ...
        
        // 100 lines: parsing logic
        std::vector<Token> tokens;
        size_t pos = 0;
        while (pos < normalized.size()) {
            // Complex tokenization logic
        }
        
        // 75 lines: AST construction
        for (const auto& token : tokens) {
            // Build abstract syntax tree
        }
        
        // 30 lines: validation and finalization
        if (!validate_ast(result.ast)) {
            // error handling
        }
        
        return result;
    }
};
```

**Problems:**
- Cannot reuse tokenization in isolation
- Cannot override just normalization step
- Must copy entire method to change one step
- Testing requires exercising entire pipeline

### 2.2 Solution: Decompose into Protected Virtual Steps

Break the algorithm into stages, each in its own method:

```cpp
class Parser {
public:
    Result parse(const std::string& input) {
        validate_input(input);
        std::string normalized = normalize(input);
        auto tokens = tokenize(normalized);
        auto ast = build_ast(tokens);
        validate_ast(ast);
        return finalize(std::move(ast));
    }

protected:
    // Each step is small, focused, and overridable
    virtual void validate_input(const std::string& input) {
        if (input.empty()) {
            throw std::runtime_error("Input cannot be empty");
        }
    }
    
    virtual std::string normalize(const std::string& input) {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    virtual std::vector<Token> tokenize(const std::string& normalized) {
        std::vector<Token> tokens;
        // Tokenization logic
        return tokens;
    }
    
    virtual AST build_ast(const std::vector<Token>& tokens) {
        AST ast;
        // AST construction
        return ast;
    }
    
    virtual void validate_ast(const AST& ast) {
        // Validation logic
    }
    
    virtual Result finalize(AST&& ast) {
        return Result(std::move(ast));
    }
};
```

**Benefits:**
- Each method has single responsibility
- Users override only what they need
- Library methods become reusable building blocks
- Easy to test each step independently

### 2.3 User Extension Example

Now users can precisely customize behavior:

```cpp
class JsonReferenceParser : public Parser {
protected:
    // Only override the normalization step to handle $ref
    std::string normalize(const std::string& input) override {
        std::string normalized = Parser::normalize(input);
        return resolve_references(normalized);
    }
    
private:
    std::string resolve_references(const std::string& json) {
        // Custom logic to expand JSON references
        // Can reuse all other Parser methods!
        return expanded;
    }
};
```

### 2.4 Small Functions with Strategy Injection

Combine decomposition with strategies for maximum flexibility:

```cpp
class Parser {
public:
    // Inject strategies for specific steps
    explicit Parser(
        std::shared_ptr<INormalizer> normalizer = default_normalizer(),
        std::shared_ptr<ITokenizer> tokenizer = default_tokenizer())
        : normalizer_(std::move(normalizer))
        , tokenizer_(std::move(tokenizer)) {}
    
    Result parse(const std::string& input) {
        validate_input(input);
        std::string normalized = normalizer_->normalize(input);
        auto tokens = tokenizer_->tokenize(normalized);
        auto ast = build_ast(tokens);
        validate_ast(ast);
        return finalize(std::move(ast));
    }

protected:
    // Core steps that are rarely changed - still overridable
    virtual void validate_input(const std::string& input) { /*...*/ }
    virtual AST build_ast(const std::vector<Token>& tokens) { /*...*/ }
    virtual void validate_ast(const AST& ast) { /*...*/ }
    virtual Result finalize(AST&& ast) { /*...*/ }
    
private:
    std::shared_ptr<INormalizer> normalizer_;
    std::shared_ptr<ITokenizer> tokenizer_;
};
```

Users choose between inheritance and composition based on their needs.

### 2.5 Guidelines for Function Decomposition

1. **Extract logical steps** - Each phase of an algorithm gets its own function
2. **Name by intent** - `normalize()`, not `do_step_2()`
3. **Minimize state sharing** - Pass data between steps explicitly
4. **Make reusable parts protected** - Let subclasses access helpers
5. **Keep public API minimal** - Internal decomposition doesn't bloat interface
6. **Consider const correctness** - Mark read-only steps as `const`

**Example: Size guidelines**
- Small: 5-15 lines (a single logical operation)
- Medium: 15-40 lines (coordinating a few operations)
- Large: 40+ lines (consider decomposition)

---

## 3. Access Control: The Strong Contract Dilemma

The choice between `private`, `protected`, and `public` fundamentally shapes a library's extensibility. This section explores the "strong contract" philosophy and its implications for power users.

### 3.1 The Traditional View: Private by Default

Classical OOP teaching emphasizes strong encapsulation:

```cpp
class DataStore {
public:
    void insert(const Record& record) {
        if (!validate(record)) throw std::invalid_argument("invalid record");
        do_insert(record);
        update_index(record);
    }

private:
    bool validate(const Record& record) { /*...*/ }
    void do_insert(const Record& record) { /*...*/ }
    void update_index(const Record& record) { /*...*/ }
    
    std::vector<Record> data_;
    std::map<int, size_t> index_;
};
```

**Strong Contract Arguments:**
- Prevents misuse by subclasses
- Free to refactor internals without breaking derived classes
- Maintains invariants (index always matches data)
- Clear API surface: only `public` methods matter

**The Power User Problem:**
This works until someone needs to:
- Optimize insertion for bulk operations
- Add a secondary index
- Instrument internal operations for profiling
- Hook validation for custom types

With everything `private`, they must:
1. Fork the library
2. Maintain patches across updates
3. Duplicate 99% of the code to change 1%

### 3.2 The Extensibility View: Protected Hooks

An extensible design provides access points:

```cpp
class DataStore {
public:
    void insert(const Record& record) {
        if (!validate_record(record)) {
            throw std::invalid_argument("invalid record");
        }
        do_insert_impl(record);
        update_indices(record);
    }

protected:
    // Validation hook - users can override
    virtual bool validate_record(const Record& record) {
        return !record.key().empty();
    }
    
    // Core insertion - subclasses can optimize
    virtual void do_insert_impl(const Record& record) {
        data_.push_back(record);
    }
    
    // Index maintenance - extensible for additional indices
    virtual void update_indices(const Record& record) {
        index_[record.key()] = data_.size() - 1;
    }
    
    // Access to data for derived classes
    std::vector<Record>& data() { return data_; }
    const std::vector<Record>& data() const { return data_; }

private:
    std::vector<Record> data_;
    std::map<int, size_t> index_;
};
```

**Benefits:**
- Users customize specific behaviors
- Core invariants still maintained (public API enforces validate → insert → index)
- No forking needed
- Library evolution continues to work for custom subclasses

**Trade-offs:**
- Larger "protected surface" to maintain
- Derived classes might violate internal assumptions
- Requires documentation of contracts

### 3.3 The Spectrum of Access Control

Different parts of a library warrant different protection levels:

| Category | Access | Rationale | Example |
|----------|--------|-----------|---------|
| **Pure Implementation Details** | `private` | Will never be needed externally; exposing adds no value | Buffer management, temporary calculations |
| **Extension Points** | `protected virtual` | Known variation points | Validation rules, formatting, resolution logic |
| **Reusable Helpers** | `protected` non-virtual | Useful for derived classes but not polymorphic | Parsing utilities, common calculations |
| **Stable Public API** | `public` | Contract with all users | Main functionality |
| **Internal but Accessible** | `public` + documentation | Advanced use with caveats | Direct iterator access, unsafe fast paths |

### 3.4 Power User Access: A Pragmatic Approach

Rather than saying "`private` incapacitates users," recognize three user categories:

**1. Regular Users (95%)**
- Use public API only
- Want simplicity and safety
- Benefit from strong contracts

**2. Power Users (4%)**
- Need to extend for specific domains
- Willing to understand internals
- Served by `protected` hooks and clear extension patterns

**3. Library Developers (<1%)**
- Maintain or fork
- Need everything
- Would access privates anyway (via `#define private public` or memcpy)

**Design for all three:**

```cpp
class ConfigurableDataStore {
public:
    // Simple API for regular users
    void insert(const Record& record) {
        insert_with_validation(record);
    }
    
    // Power user API - explicit opt-in to complexity
    void insert_bulk_unsafe(std::vector<Record>&& records) {
        data_.insert(data_.end(), 
                     std::make_move_iterator(records.begin()),
                     std::make_move_iterator(records.end()));
        rebuild_indices();  // Exposed for bulk operations
    }

protected:
    // Extension points for power users
    virtual bool validate_record(const Record& record) {
        return !record.key().empty();
    }
    
    virtual void do_insert_impl(const Record& record) {
        data_.push_back(record);
    }
    
    // Exposed for custom index implementations
    virtual void rebuild_indices() {
        index_.clear();
        for (size_t i = 0; i < data_.size(); ++i) {
            index_[data_[i].key()] = i;
        }
    }
    
    // Helper accessible to derived classes
    std::vector<Record>& data_storage() { return data_; }

private:
    // Truly internal - no legitimate reason to access
    std::vector<Record> data_;
    std::map<int, size_t> index_;
    
    void insert_with_validation(const Record& record) {
        if (!validate_record(record)) {
            throw std::invalid_argument("invalid record");
        }
        do_insert_impl(record);
        update_single_index(record);
    }
    
    void update_single_index(const Record& record) {
        index_[record.key()] = data_.size() - 1;
    }
};
```

### 3.5 When to Use `final`

The `final` keyword prevents subclassing entirely. Use it when:

1. **Security/Safety Critical** - Subclassing could violate critical invariants
2. **Optimization Enabled** - Compiler can devirtualize and inline
3. **Alternative Extension Provided** - Composition/strategies available instead
4. **Value Types** - Small types that should be passed by value

**Don't use `final` when:**
- "I don't want to think about inheritance" (lazy reasoning)
- "This makes the API smaller" (it doesn't; subclassing is opt-in)
- "Users might do it wrong" (document correct usage instead)

**Example: Appropriate use of `final`**

```cpp
// Value type - never meant for polymorphism
class Point final {
public:
    Point(double x, double y) : x_(x), y_(y) {}
    double x() const { return x_; }
    double y() const { return y_; }
private:
    double x_, y_;
};

// Security boundary - subclassing could bypass authentication
class SecureConnection final : public Connection {
public:
    void send(const Message& msg) override {
        if (!authenticated_) throw std::runtime_error("not authenticated");
        Connection::send(msg);
    }
private:
    bool authenticated_ = false;
};

// But the factory is still extensible!
class ConnectionFactory {
public:
    virtual std::unique_ptr<Connection> create(const Config& cfg) {
        if (cfg.requires_security()) {
            return std::make_unique<SecureConnection>(cfg);
        }
        return std::make_unique<BasicConnection>(cfg);
    }
};
```

### 3.6 Documentation: The Missing Piece

Access control alone isn't enough. Document the contract:

```cpp
class Parser {
protected:
    /**
     * Normalize input string before tokenization.
     * 
     * Default implementation: converts to lowercase.
     * 
     * Contract for overrides:
     * - Must return valid UTF-8
     * - Must not throw unless input is fundamentally invalid
     * - May modify character case, whitespace, etc.
     * - Should be idempotent: normalize(normalize(x)) == normalize(x)
     * 
     * Called by: parse() before tokenize()
     * 
     * @param input Raw input string
     * @return Normalized string ready for tokenization
     */
    virtual std::string normalize(const std::string& input);
};
```

Clear contracts let power users extend confidently without breaking invariants.

### 3.7 Recommendations Summary

1. **Default to `private` for pure implementation details** - No external value
2. **Use `protected` for known extension points** - Enable power users
3. **Document protected contracts carefully** - Explain invariants and expectations
4. **Provide `public` unsafe/advanced APIs** when appropriate - With clear warnings
5. **Reserve `final` for valid reasons** - Not as a default
6. **Test extension scenarios** - Write tests that subclass and verify contracts hold
7. **Offer both inheritance and composition** - Let users choose

---

## 4. Summary: Principles in Practice

Extensible library design isn't about making everything public or avoiding encapsulation. It's about **intentional openness**:

1. **Factory Pattern** enables injection of custom implementations
   - Classical: Virtual interfaces + runtime registration
   - Templates: Traits and policies for zero-overhead
   - Hybrid: Type erasure for flexibility without virtual in API

2. **Small Functions** maximize reusability
   - Break algorithms into logical steps
   - Make each step a focused, testable unit
   - Protected methods let users override specific behaviors
   - Combine with strategies for ultimate flexibility

3. **Thoughtful Access Control** balances safety and extensibility
   - Private for true implementation details
   - Protected for extension points and reusable helpers
   - Document contracts for protected interfaces
   - Use `final` sparingly and with justification

**The Payoff:**
A well-designed extensible library serves regular users with a clean, simple API while enabling power users to adapt to unforeseen requirements—without forking. This is the hallmark of mature library design.

---

## Appendix: Quick Reference

### Factory Pattern Checklist
- [ ] Define abstract interface for extensible components
- [ ] Provide factory/registry for runtime or traits for compile-time
- [ ] Include default implementations
- [ ] Document extension mechanism with examples
- [ ] Test that custom implementations work without library modification

### Function Decomposition Checklist
- [ ] Each method has single, clear responsibility
- [ ] Method names describe intent, not implementation
- [ ] Complex algorithms broken into 5-15 line functions
- [ ] Protected methods expose reusable building blocks
- [ ] Public API remains minimal and stable

### Access Control Checklist
- [ ] Private for implementation details with no external value
- [ ] Protected for extension points and derived class helpers
- [ ] Virtual for known variation points
- [ ] Final only when justified (security, optimization, value semantics)
- [ ] Protected contracts documented clearly
- [ ] Extension scenarios covered in tests


Library authors rarely know all future use cases. The highest leverage design choice you can make early is leaving deliberate, well‑framed extension seams. When you freeze architecture through pervasive `final` classes, non-virtual construction, rigid factories, or deep hiding in a `detail` namespace without sanctioned customization points, you convert every unconventional user need into a fork. Fork pressure is a symptom of insufficient extensibility, not “power users being demanding.”

Below is a guided tour of principles, common anti‑patterns (illustrated by a JSONPath selector design), and practical patterns for building libraries that are stable yet open.

---

## 1. Problem Statement: The Cost of Closed Designs

A closed design:
- Forces copy/paste modification or forking to add behaviors.
- Prevents experimentation (e.g., performance tweaks, instrumentation, tracing).
- Blocks integration with domain features (security filters, caching, cross-cutting reference resolution).
- Increases long-term maintenance burden (users carry patches; authors field feature requests duplicating near‑miss variants).

In many JSONPath implementations, selector types (like `identifier_selector`, `filter_selector`, etc.) are marked `final`, directly inheriting from a non‑abstract base with concrete traversal logic. Instantiation is hard-coded; there’s no pluggable factory or registry for substituting alternative selector subclasses. Even if you remove `final` locally, users must edit source—no external extension path.

---

## 2. Guiding Principles of Extensible Library Design

1. Inversion of Control (IoC): Let callers supply strategy objects rather than hard-coding decisions.
2. Open/Closed Principle (OCP): Classes should be open for extension but closed for breaking modification. You achieve this via stable interfaces, not blanket `final`.
3. Stable Interfaces, Experimental Implementations: Define narrow abstract bases; allow multiple concrete implementations (including user ones).
4. Separation of Policy and Mechanism: Mechanisms (traversal, parsing) are reusable; policies (resolution rules, indexing semantics) are swappable (Strategy Pattern or template policy parameters).
5. Layered Encapsulation: Encapsulation is valuable—but apply it to invariants, not to every class. Private data is fine; sealing entire types to block any customization is not.
6. Extensibility Budget: Identify “hot spots” (likely user variation) early: allocation strategy, logging hook, reference resolution, predicate evaluation, indexing, caching.

---

## 3. Anti-Patterns Observed

| Anti-Pattern | Effect | Remedy |
|--------------|--------|--------|
| `final` on every leaf class | No subclassing for small tweaks | Reserve `final` only for truly invariant classes or ones offering alternative extension via composition |
| Hidden construction (direct `new` in parser) | No interception or replacement | Introduce virtual factory / registry |
| Monolithic base class with many overridables but leaf classes sealed | Paradox: interface suggests polymorphism but sealing blocks it | Either truly seal and expose composable strategies, or unseal for subclassing |
| Tight coupling to JSON concrete type | Harder to add adaptation layers (e.g., lazy nodes, external refs) | Use type-erasure interface or concepts |
| Reference resolution hard-coded inside a selector | Users cannot plug custom resolution logic | Lift to a strategy (function object, virtual interface, or template policy) |

> Note on language keywords: `final`, `private`, and `protected` are powerful tools—but overusing them without providing alternative extension mechanisms effectively incapacitates library users. If the goal is to preserve invariants, expose explicit extension seams (strategies, hooks, factories) while keeping internal state private. Avoid sealing off types that users reasonably need to adapt.

---

## 4. Extension Patterns That Work

### 4.1 Virtual Interface + Factory Registration
Define an abstract base for selectors and a factory capable of constructing them by token kind.

```cpp
class ISelector {
public:
    virtual ~ISelector() = default;
    virtual void select(EvalContext&, Node&, Receiver&) const = 0;
    virtual std::string debug() const = 0;
};

using SelectorFactoryFn = std::unique_ptr<ISelector>(*)(const Token&);

class SelectorRegistry {
public:
    static SelectorRegistry& instance() {
        static SelectorRegistry r; return r;
    }
    void register_factory(TokenKind kind, SelectorFactoryFn fn) {
        factories_[kind] = fn;
    }
    std::unique_ptr<ISelector> create(const Token& tok) const {
        auto it = factories_.find(tok.kind());
        if (it == factories_.end()) throw std::runtime_error("Unknown selector");
        return (it->second)(tok);
    }
private:
    std::unordered_map<TokenKind, SelectorFactoryFn> factories_;
};
```

Users register new behaviors without modifying core code.

### 4.2 Strategy Injection
Instead of hard-coding reference resolution, accept it as a strategy:

```cpp
class IReferenceResolver {
public:
    virtual ~IReferenceResolver() = default;
    virtual Json resolve(const Json& root, const Json& candidate) const = 0;
};

class IdentifierSelector : public ISelector {
public:
    IdentifierSelector(std::string id,
                       std::shared_ptr<IReferenceResolver> resolver)
        : id_(std::move(id)), resolver_(std::move(resolver)) {}
    void select(EvalContext& ctx, Node& current, Receiver& r) const override {
        // Lookup, then:
        if (isRef(node)) {
            auto resolved = resolver_->resolve(ctx.root(), node);
            // continue traversal with resolved
        }
    }
private:
    std::string id_;
    std::shared_ptr<IReferenceResolver> resolver_;
};
```

### 4.3 Template Policy (Zero-Overhead Customization)

```cpp
template<class ReferenceResolverPolicy>
class IdentifierSelectorT : public ISelector {
public:
    IdentifierSelectorT(std::string id, ReferenceResolverPolicy resolver)
        : id_(std::move(id)), resolver_(std::move(resolver)) {}
    void select(EvalContext& ctx, Node& current, Receiver& r) const override {
        // uses resolver_.resolve(...)
    }
private:
    std::string id_;
    ReferenceResolverPolicy resolver_;
};
```

Users supply a lightweight policy struct; no virtual dispatch if they don’t need dynamic polymorphism.

### 4.4 Type Erasure for Flexibility
If you don’t want virtual exposure for every strategy:

```cpp
class ReferenceResolver {
public:
    template<typename F>
    ReferenceResolver(F f) : self_(std::make_shared<Model<F>>(std::move(f))) {}
    Json resolve(const Json& root, const Json& cand) const {
        return self_->resolve(root, cand);
    }
private:
    struct Concept { virtual ~Concept()=default; virtual Json resolve(const Json&, const Json&) const = 0; };
    template<typename F>
    struct Model : Concept {
        F fn_;
        Model(F f):fn_(std::move(f)){}
        Json resolve(const Json& r, const Json& c) const override { return fn_(r,c); }
    };
    std::shared_ptr<const Concept> self_;
};
```

### 4.5 Event Hook / Observer
Allow users to hook traversal steps (before/after node visited). This provides extension without subclassing.

---

## 5. Case Study: Opening a JSONPath Selector Architecture

Current obstacles commonly seen:
- Every selector declared `final` (`identifier_selector`, `filter_selector`, etc.).
- Construction logic embedded; no documented injection seam.
- Reference resolution internal to a leaf selector.
- No public stable abstract selector contract that third parties can implement.

Recommended staged refactor:

1. Introduce a public (non-`detail`) `ISelector` (or promote an internal base) as a stable extension interface.
2. Remove `final` from selectors OR replace subclass need with parameterized strategies (preferred if invariants must hold).
3. Factor out reference resolution into a strategy interface.
4. Create a registry-based factory; expose registration API.
5. Provide default built-in selector registrations so existing behavior stays intact.
6. Document extension workflow (add new selector kind, plug in via registry, use in expression).

Minimal diff conceptually (simplified):

```cpp
// Before
class identifier_selector final : public base_selector<Json, JsonRef> { /*...*/ };

// After
class identifier_selector : public base_selector<Json, JsonRef> {
public:
    identifier_selector(string_type id,
                        std::shared_ptr<IReferenceResolver> resolver = default_reference_resolver())
        : identifier_(std::move(id)), resolver_(std::move(resolver)) {}
protected: // intentional hooks
    virtual bool is_reference(reference v) const { /*...*/ }
    virtual value_type resolve_reference(reference root, reference v) const {
        return resolver_->resolve(root,v);
    }
private:
    std::shared_ptr<IReferenceResolver> resolver_;
};
```

---

## 6. Balancing Encapsulation vs. Extensibility

It’s tempting to say “keywords like `final`, `private`, `protected` incapacitate users.” The truth is nuanced:

- `private`: Protects invariants. Keep internal state private; expose extension points instead of state poking.
- `protected`: Use sparingly as intentional hooks when subclassing is a first-class extension path.
- `final`: Use where multiple implementations provide no value and internal logic must not be redefined. Don’t blanket `final` to reduce surface area—provide composition alternatives first.

Conclusion: These keywords aren’t inherently bad; indiscriminate use without alternative extension channels causes rigidity. If you choose to restrict subclassing, provide replaceable factories, strategies, or hooks so users aren’t boxed in.

---

## 7. Patterns Cheat Sheet

| Goal | Pattern | Notes |
|------|---------|-------|
| Swap behavior at runtime | Strategy (virtual) | Good for dynamic plugin loading |
| Zero-cost customization | Template Policy (CRTP / traits) | Compile-time; version compatibility risk |
| External contributions | Registry + Factory | Supports late binding |
| Reduced ABI exposure | Pimpl + interfaces | Keep binary compatibility while allowing extension |
| Multiple orthogonal behaviors | Composition of small interfaces | Single Responsibility applied to extension points |
| Experimental user code | Type Erasure boundary | Avoid templating explosion |

---

## 8. Testing Extensibility

Provide harness tests proving extension seams work:

```cpp
struct LoggingResolver : IReferenceResolver {
    Json resolve(const Json& root, const Json& candidate) const override {
        std::clog << "Resolving " << candidate << "\n";
        return default_impl(root, candidate); // call library helper
    }
};

// User registers custom selector
SelectorRegistry::instance().register_factory(TokenKind::Identifier,
    [](const Token& t){
        return std::make_unique<IdentifierSelector>(t.text(),
            std::make_shared<LoggingResolver>());
    });
```

A test ensures traversal invokes custom logic without modifying core files.

---

## 9. Migration Strategy for Existing Libraries

1. Identify hot spot classes requested for customization.
2. Introduce abstract interfaces in a minor version; keep old constructors but mark as deprecated.
3. Refactor internal creation to go through the new factory.
4. Provide adapters so old API surfaces still function.
5. Publish “Extensibility Guide” with examples.
6. Gather feedback on extension usage (optional).

---

## 10. When NOT to Make Things Extensible

Extensibility has costs. Sometimes the rigid, `final` approach is the right choice:

### 10.1 Performance-Critical Inner Loops

**When:**
- Hot path in real-time or embedded systems
- Proven bottleneck through profiling
- Microseconds matter

**Why Restrict:**
- Virtual dispatch costs 5-10 nanoseconds per call
- Prevents inlining and other optimizations
- Template instantiation can bloat code size

**Example:**
```cpp
// Matrix multiplication inner loop - keep it final and inline
class DenseMatrix final {
public:
    double operator()(size_t i, size_t j) const {
        return data_[i * cols_ + j];  // Direct access, fully inlinable
    }
private:
    std::vector<double> data_;
    size_t cols_;
};
```

### 10.2 Security Boundaries

**When:**
- Authentication/authorization logic
- Cryptographic operations
- Input validation that prevents attacks

**Why Restrict:**
- Subclassing could bypass security checks
- Invariants are security-critical

**Example:**
```cpp
class SecureChannel final {
public:
    void send(const Message& msg) {
        if (!authenticated_) throw SecurityException("not authenticated");
        if (!validate_hmac(msg)) throw SecurityException("tampered message");
        transport_->send(encrypt(msg));
    }
private:
    bool authenticated_;
    // Cannot allow subclass to skip authentication
};
```

### 10.3 Simple Value Types

**When:**
- POD-like types (Point, Color, Complex)
- Meant to be passed by value
- No polymorphic behavior makes sense

**Example:**
```cpp
struct Point final {
    double x, y;
    double distance_to(const Point& other) const;
};
```

### 10.4 Cost-Benefit Analysis by Library Scale

| Library Size | User Base | Extensibility Recommendation |
|--------------|-----------|------------------------------|
| **Small (<5K LOC)** | Internal team | Keep simple; add extensibility when requested |
| **Medium (5-50K)** | Multiple teams | Plan extension points; document patterns |
| **Large (>50K)** | Public/open-source | Extensibility is critical; test extension scenarios |
| **Framework** | Ecosystem of plugins | Extensibility is the primary feature |

### 10.5 The Extensibility Decision Tree

```
Does the class have multiple valid implementations?
├─ No → Consider `final` or simple concrete class
└─ Yes
    ├─ Is it performance-critical? (profiled)
    │   ├─ Yes → Template policy or keep final
    │   └─ No → Continue
    ├─ Do users need runtime selection?
    │   ├─ Yes → Virtual interface + factory
    │   └─ No → Template policy
    └─ Is ABI stability important?
        ├─ Yes → Pimpl + stable interface
        └─ No → Header-only templates OK
```

**Key Principle:** Default to extensibility for public libraries, but be willing to restrict when there's clear justification (performance, security, simplicity). Document the reasoning.

---

## 11. Case Study: Refactoring a Closed Parser

Let's see these principles applied to a realistic example.

### 11.1 Original Design (Closed)

```cpp
// Monolithic, sealed design
class JSONParser final {
public:
    JSON parse(const std::string& input) {
        // 200 lines of intermingled logic:
        // - validation
        // - tokenization  
        // - parsing
        // - AST construction
        // All in one method, all private helpers
    }
private:
    void validate(const std::string& input);
    Token next_token();
    // ... 15 more private methods
};
```

**Problems:**
- Cannot customize validation (e.g., relaxed mode)
- Cannot inject custom number parsing (e.g., BigDecimal)
- Cannot add instrumentation
- Cannot reuse tokenizer standalone
- To add JSON Reference resolution: **must fork**

### 11.2 Refactored Design (Extensible)

**Step 1: Extract interfaces**

```cpp
class ITokenizer {
public:
    virtual ~ITokenizer() = default;
    virtual std::vector<Token> tokenize(const std::string& input) = 0;
};

class IASTBuilder {
public:
    virtual ~IASTBuilder() = default;
    virtual JSON build(const std::vector<Token>& tokens) = 0;
};
```

**Step 2: Decompose parser class**

```cpp
class JSONParser {
public:
    JSONParser(std::shared_ptr<ITokenizer> tokenizer = default_tokenizer(),
               std::shared_ptr<IASTBuilder> builder = default_builder())
        : tokenizer_(std::move(tokenizer))
        , builder_(std::move(builder)) {}
    
    JSON parse(const std::string& input) {
        validate_input(input);
        auto tokens = tokenizer_->tokenize(input);
        auto ast = builder_->build(tokens);
        validate_result(ast);
        return ast;
    }

protected:
    // Extension points
    virtual void validate_input(const std::string& input) {
        if (input.empty()) throw std::runtime_error("empty input");
    }
    
    virtual void validate_result(const JSON& result) {
        // Schema validation, etc.
    }

private:
    std::shared_ptr<ITokenizer> tokenizer_;
    std::shared_ptr<IASTBuilder> builder_;
};
```

**Step 3: Provide default implementations**

```cpp
class StandardTokenizer : public ITokenizer {
public:
    std::vector<Token> tokenize(const std::string& input) override {
        std::vector<Token> tokens;
        // Standard tokenization
        return tokens;
    }
};

class StandardASTBuilder : public IASTBuilder {
public:
    JSON build(const std::vector<Token>& tokens) override {
        JSON result;
        // Standard AST construction
        return result;
    }
};
```

### 11.3 User Extensions Now Possible

**Use case 1: Add JSON Reference resolution**

```cpp
class ReferenceResolvingBuilder : public StandardASTBuilder {
public:
    JSON build(const std::vector<Token>& tokens) override {
        JSON ast = StandardASTBuilder::build(tokens);
        return resolve_references(ast);
    }
    
private:
    JSON resolve_references(const JSON& ast) {
        // Walk AST, resolve $ref pointers
        return resolved;
    }
};

// Use it
JSONParser parser(default_tokenizer(), 
                  std::make_shared<ReferenceResolvingBuilder>());
```

**Use case 2: Relaxed validation**

```cpp
class RelaxedJSONParser : public JSONParser {
protected:
    void validate_input(const std::string& input) override {
        // Allow trailing commas, comments, etc.
        // No exception for minor issues
    }
};
```

**Use case 3: Performance instrumentation**

```cpp
class InstrumentedTokenizer : public ITokenizer {
public:
    InstrumentedTokenizer(std::shared_ptr<ITokenizer> inner)
        : inner_(std::move(inner)) {}
        
    std::vector<Token> tokenize(const std::string& input) override {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = inner_->tokenize(input);
        auto duration = std::chrono::high_resolution_clock::now() - start;
        
        std::cout << "Tokenization: " << duration.count() << "ns\n";
        return result;
    }
    
private:
    std::shared_ptr<ITokenizer> inner_;
};
```

### 11.4 Impact Summary

| Metric | Before | After |
|--------|--------|-------|
| Fork required for extension | Yes | No |
| User code changes for new feature | 100K+ lines | <100 lines |
| Reusable components | 0 | Tokenizer, Builder separately usable |
| Test isolation | Must test entire parser | Can test components independently |
| Breaking changes when refactoring | High risk | Low risk (internal changes don't affect users) |

**Code size cost:** ~150 lines of interface/factory code (one-time cost)  
**Benefit:** Unlimited extension possibilities without library modification

---

## 12. Practical Checklist for Authors

- Are all “leaf” classes truly invariant? If not, remove `final`.
- Do users need to inject behavior? Provide interface + factory or policy template.
- Is there at least one documented example of writing a plugin?
- Can a user extend without editing your namespace or patching source?
- Are upgrade paths clear (semantic versioning, stable abstract APIs)?
- Do tests cover both built-in and custom extension scenarios?

---

## 13. Summary: Principles in Practice

Good C++ library design is about **purposeful openness**—expose intentional seams for behavior, keep invariants private, and avoid sealing all types without justification.

### Core Takeaways

1. **Factory Pattern** enables injection of custom implementations
   - Classical: Virtual interfaces + runtime registration for dynamic flexibility
   - Templates: Traits and policies for zero-overhead customization
   - Hybrid: Type erasure when you need both

2. **Small Functions** maximize reusability
   - Break algorithms into 5-15 line focused steps
   - Make extension points protected and virtual
   - Each method has single responsibility
   - Users override only what they need

3. **Thoughtful Access Control** balances safety and extensibility
   - Private for true implementation details
   - Protected for extension points and reusable helpers
   - Document contracts for protected interfaces
   - Use `final` sparingly—only with clear justification

4. **Know When to Restrict**
   - Performance-critical paths (after profiling)
   - Security boundaries
   - Simple value types
   - Scale appropriately to library size

### The Cost of Closed Design

Without these principles, users face:
- Forking entire codebases for small features
- Duplicating maintenance effort
- Inability to experiment or integrate domain features
- Frustration and abandonment

### The Benefit of Extensible Design

With deliberate extensibility:
- Users extend without touching your source
- Features emerge that you never imagined
- Testing becomes easier (mock implementations)
- Library evolution doesn't break customizations
- Community contributions increase

### Final Thought

Extensibility is not feature bloat; it's **deferred adaptability**. A modest investment in extension seams—abstract interfaces, replaceable factories, decomposed functions—unlocks unlimited customization without destabilizing existing code.

Design for the unknown. Empower your users.

- Introduce `ISelector` publicly.
- Provide `IReferenceResolver`.
- Implement registry API and migrate parser to use it.
- Publish migration + extension docs.

---

## 14. Appendix: Quick Reference

### Factory Pattern Checklist
- [ ] Define abstract interface for extensible components
- [ ] Provide factory/registry for runtime or traits for compile-time
- [ ] Include default implementations
- [ ] Document extension mechanism with examples
- [ ] Test that custom implementations work without library modification

### Function Decomposition Checklist
- [ ] Each method has single, clear responsibility
- [ ] Method names describe intent, not implementation
- [ ] Complex algorithms broken into 5-15 line functions
- [ ] Protected methods expose reusable building blocks
- [ ] Public API remains minimal and stable

### Access Control Checklist
- [ ] Private for implementation details with no external value
- [ ] Protected for extension points and derived class helpers
- [ ] Virtual for known variation points
- [ ] Final only when justified (security, optimization, value semantics)
- [ ] Protected contracts documented clearly
- [ ] Extension scenarios covered in tests
