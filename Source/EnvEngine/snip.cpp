



int Classify(float val, float minVal, float maxVal, int binCount) {
    val = (val - minVal) / (maxVal - minVal);  // [0-1]
    int bin = Math.floor(val * binCount);
    if (bin > binCount - 1)
        bin = binCount - 1;
    if (bin < 0)
        bin = 0;
    return bin;
}

//long MapColor(float val, float minVal, float maxVal, colors) {
//    var count = colors.length;
//    var bin = Classify(val, minVal, maxVal, count);
//    return colors[bin];
//}

//function MakeRange(start, end) {
//    return Array.from({ length: (end - start) }, (v, k) => k + start);
//}





    constructor() { }

    // private methods
    LoadPreBuildSettings(settings) {
        if ("input" in settings) {
            this.inputType = settings.input.type;  // global

            switch (this.inputType) {
                case "constant":
                    this.k = settings.input.value;
                    break;
                case "constant_with_stop":
                    this.k1 = settings.input.value;
                    this.stop = settings.input.stop;
                    break;
                case "sinusoidal":
                    this.amp = settings.input.amplitude;
                    this.period = settings.input.period;
                    this.phase = settings.input.phase_shift;
                    break;
                case "random":
                    break;
                case "track_output":
                    break;
            }
        }

        if ("trans_eff_max" in settings)
            this.transEffMax = settings.trans_eff_max;

        if ("agg_input_sigma" in settings)
            this.aggInputSigma = settings.agg_input_sigma;

        if ("reactivity_steepness_factor_B" in settings)
            this.activationSteepFactB = settings.reactivity_steepness_factor_B;

        if ("reactivity_threshold" in settings)
            this.activationThreshold = settings.reactivity_threshold;

        if ("autogenerate_landscape_signals" in settings) {
            if (typeof settings.autogenerate_landscape_signals === 'boolean') {
                if (this.autogenFraction === true)
                    this.autogenFraction = 1.0;
                else
                    this.autogenFraction = 0;
            } else {    // it's an object
                this.autogenFraction = settings.autogenerate_landscape_signals.fraction;
                this.autogenBias = settings.autogenerate_landscape_signals.bias;
            }
        }

        if ("autogenerate_transit_times" in settings) {
            this.autogenTransTimeMax = settings.autogenerate_transit_times.max_transit_time;
            this.autogenTransTimeBias = settings.autogenerate_transit_times.bias;
        }

        if ("layout" in settings) {
            this.layout = settings.layout;
            this.UpdateNetworkLayout(settings.layout);
        }

        if ("infSubmodel" in settings) {
            switch (settings.infSubmodel) {
                case "sender_receiver":
                    this.infSubmodel = IM_SENDER_RECEIVER;
                    break;
                case "signal_receiver":
                    this.infSubmodel = IM_SIGNAL_RECEIVER;
                    break;
                case "signal_sender_receiver":
                    this.infSubmodel = IM_SIGNAL_SENDER_RECEIVER;
                    break;
            }
        }

        if ("infSubmodelWt" in settings) {
            this.infSubmodelWt = settings.infSubmodelWt;
        }
    }
  
    


    AddAutogenInputEdges() {
        if (this.autogenFraction > 0) {
            // add landscape signal edges
            let sourceNode = this.cy.nodes(INPUT_SIGNAL)[0];
            let sourceID = sourceNode.data('id');
            let sourceTraits = sourceNode.data('traits');
            let threshold = 0;
            let index = 0;
            let snodes = null;
            let targetID = null;
            switch (this.autogenBias) {
                case "influence":
                case "reactivity":
                    snodes = this.cy.nodes().sort(function (a, b) {
                        return b.data(this.autogenBias) - a.data(this.autogenBias);  // decreasing order
                    });

                    threshold = snodes.length * this.autogenFraction;
                    for (index = 0; index < threshold; index++) {
                        targetID = snodes[index].data('id');
                        this.AddAutogenInputEdge('e' + this.nextEdgeIndex, sourceID, targetID, sourceTraits, this.actorValue);
                    }
                    break;

                case "degree":
                    snodes = this.cy.nodes().sort(function (a, b) {
                        return b.degree(false) - a.degree(false);  // decreasing order
                    });

                    threshold = snodes.length * this.autogenFraction;
                    for (index = 0; index < threshold; index++) {
                        targetID = snodes[index].data('id');
                        this.AddAutogenInputEdge('e' + this.nextEdgeIndex, sourceID, targetID, sourceTraits, this.actorValue);
                    }
                    break;

                case "random":
                    this.cy.nodes().forEach(function (node, index) {
                        let rand = this.randGenerator();
                        if (rand < this.autogenFraction) {
                            if (index !== nodeCount - 1) {  // skip last node (self) ///??????????
                                targetID = this.cy.nodes().data('id'); // = 'n' + index;
                                this.AddAutogenInputEdge('e' + this.nextEdgeIndex, sourceID, targetID, sourceTraits, this.actorValue);
                            }
                        }
                    });
                    break;
            }
        }
        return;
    }

    // only landscape edges for now
    AddAutogenInputEdge(id, sourceID, targetID, sourceTraits, actorValue) {

        this.cy.add({
            group: 'edges',
            data: {
                id: id,   // FromIndex_ToIndex
                name: 'Landscape Signal',
                label: '',
                source: sourceID,

                target: targetID,
                signalStrength: 0,   // [-1,1]
                signalTraits: sourceTraits,  // traits vector for current signal, or null if inactive
                transEff: actorValue,
                transEffSender: 0,
                transEffSignal: actorValue,
                transTime: 0,
                activeCycles: 0,
                state: STATE_ACTIVE,
                influence: actorValue,
                width: 0.5,
                lineColor: 'lightblue',
                type: ET_INPUT,
                watch: 0,
                opacity: 1,
                show: 0
            }
        });
        this.nextEdgeIndex++;
    }

    SetEdgeTransitTimes() {
        if (this.autogenTransTimeMax > 0) {
            // add landscape edges (landscape node is last node added)

            let _this = this;

            switch (this.autogenTransTimeBias) {
                case "influence":
                case "transEff":
                    this.cy.edges(NETWORK_EDGE).forEach(function (edge, index) {   // ignore landscape edges
                        //let name = edge.data("name");
                        let scalar = edge.data(_this.autogenTransTimeBias);  // WRONG?????!!!!  // [-1,1] for both transEff, influence (NOTE: NOT QUITE CORRECT FOR TRANSEFF)
                        scalar = (scalar + 1) / 2;       // [0,1]
                        scalar = 1 - scalar;            // [1,0]
                        let tt = Math.round(scalar * _this.autogenTransTimeMax);
                        edge.data("transTime", tt);
                    });
                    break;

                case "random":
                    this.cy.edges(NETWORK_EDGE).forEach(function (edge, index) {
                        let rand = this.randGenerator();  // [0,1]
                        let tt = Math.round(rand * _this.autogenTransTimeMax);
                        edge.data("transTime", tt);
                    });
                    break;
            }
        }
    }

    GenerateInputArray() {
        // populate the model's netInputs vector
        let xs = MakeRange(0, this.cycles);

        switch (this.inputSeriesType) {
            case I_CONSTANT:    // constant
                this.netInputs = xs.map((x, index, array) => { return this.k; });
                break;

            case I_CONSTWSTOP:  // constant with stop
                this.netInputs = xs.map(function (x, index, array) { return (x < stop) ? this.k1 : 0; });
                break;

            case I_SINESOIDAL:  // sinesoidal
                this.netInputs = xs.map(function (x, index, array) { return 0.5 + (this.amp * Math.sin((2 * Math.PI * x / this.period) + this.phase)); });
                break;

            case I_RANDOM:  // random
                this.netInputs = xs.map(function (x, index, array) { return Math.random(); }); // [0,1] 
                break;

            case I_TRACKOUTPUT: // track output
                this.netInputs = xs.map(function (x, index, array) { return 0; });  // determined at runtime
                break;
        }
    }


   



    //------------------------------------------------------- //
    //---------------- Influence Submodel ------------------- //
    //------------------------------------------------------- //

    ComputeEdgeTransEffs(selector) {  // NOTE: Only need to do active edges
        let _this = this;
        this.cy.edges(selector).forEach(function (edge) {
            _this.ComputeEdgeTransEff(edge);
        });
    }

    ComputeEdgeTransEff(edge) {
        // transmission efficiency depends on the current network model:
        // IM_SENDER_RECEIVER - uses sender node traits
        // IM_SIGNAL_RECEIVER - uses edge signal traits
        // IM_SIGNAL_SENDER_RECEIVER - uses both
        var transEff = 0;
        var source = edge.source();   // sender node
        var target = edge.target();   // receiver node
        var srcType = source.data('type');
        let transEffSignal = 0;
        let transEffSender = 0;
        let sTraits = [];  // source/signal traits
        let tTraits = target.data('traits');  // receiver node
        let similarity = 0;
        // next, we will calulate two transmission efficiencies, 
        // 1) based on relationship between receiver traits and signal traits
        // 2) based on relationship between receiver traits and sender traits.
        // We assume all input edge traits are initialized to the input
        // message traits

        // compute similarity between signal and receiver traits, 
        // i.e. the edges 'signalTraits' and the receiver nodes 'traits' array
        sTraits = edge.data('signalTraits');
        similarity = this.ComputeSimilarity(sTraits, tTraits);
        transEffSignal = this.transEffAlpha + this.transEffBetaTraits0 * similarity;   // [0,1]

        sTraits = source.data('traits');
        similarity = this.ComputeSimilarity(sTraits, tTraits);
        transEffSender = this.transEffAlpha + this.transEffBetaTraits0 * similarity;   // [0,1]
        //}

        switch (this.infSubmodel) {
            case IM_SIGNAL_RECEIVER:
                transEff = transEffSignal;
                break;

            case IM_SENDER_RECEIVER:
                transEff = transEffSender;
                break;

            case IM_SIGNAL_SENDER_RECEIVER:
                transEff = (this.infSubmodelWt * transEffSender) + ((1 - this.infSubmodelWt) * transEffSignal);
                break;
        }

        //transEff *= edge.data('transEffMultiplier');
        edge.data('transEff', transEff);
        edge.data('transEffSignal', transEffSignal);
        edge.data('transEffSender', transEffSender);

        return transEff;
    }

    ComputeSimilarity(sTraits, rTraits) {  // sender, receiver traits
        // use only numeric attributes
        if (sTraits === null || rTraits === null) {
            this.ErrorMsg("Error", "Missing traits when computing similarity");
            return 0;
        }

        var count = 0;
        var traitsDelta = 0;
        var similarity = 0;

        for (var i = 0; i < sTraits.length; i++) {
            if (isNaN(sTraits[i]) === false && isNaN(rTraits[i]) === false) {
                traitsDelta += (sTraits[i] - rTraits[i]) * (sTraits[i] - rTraits[i]);
                count++;
            }
        }
        if (count > 0) {
            var dMax = Math.sqrt(count * 4);   // sqrt(sum(((1-(-1))^2)))summed of each dimension
            similarity = 1 - (Math.sqrt(traitsDelta) / dMax);
        }
        return similarity;
    }

    // The following two functions compute the influence that flows along each edge connecting two actors.
    // Compute edge weights (influence flows) by running the influence submodel on each connecting edge.
    // These function assume:
    //   1) edge transEff data is current
    //   2) node reactivity data is current

    ComputeEdgeInfluences(selector) {
        let _this = this;
        this.cy.edges(selector).forEach(function (edge) {
            _this.ComputeEdgeInfluence(edge);
        });
    }

    ComputeEdgeInfluence(edge) {
        let transEff = edge.data('transEff');
        let influence = 0;
        let srcNode = null;
        let srcReactivity = 0;

        console.log("sub", this.infSubmodel)
        console.log("transEff", edge.data('transEff'))

        switch (this.infSubmodel) {
            case IM_SIGNAL_RECEIVER:
                // influence is the signal strength of the edge * transission efficiency
                influence = edge.data('signalStrength') * transEff;
                break;

            case IM_SENDER_RECEIVER:
                // influence is the reactivity of the source * transmission efficiency
                srcNode = edge.source();
                srcReactivity = srcNode.data('reactivity');
                influence = transEff * srcReactivity;
                break;

            case IM_SIGNAL_SENDER_RECEIVER:
                // blend the above
                srcNode = edge.source();
                console.log(edge.data('transEffSender'), edge.data('transEffSignal'), "transeffs, sender - signal")
                let influenceSender = srcNode.data('reactivity') * edge.data('transEffSender');
                let influenceSignal = edge.data('signalStrength') * edge.data('transEffSignal');
                console.log(influenceSender,influenceSignal,"calculate influece")

                influence = (this.infSubmodelWt * influenceSender) + ((1 - this.infSubmodelWt) * influenceSignal);
                break;
        }

        edge.data('influence', influence);
        if (this.OnWatchMsg !== null)
            this.OnWatchMsg(edge, "Edge " + edge.data('name') + ": Influence=" + influence.toFixed(3));

        console.log("ComputeEdgeInfluence: ", edge.data('name'), " - ", edge.data('influence'))
        return influence;
    }

    ActivateNode(node, bias) {
        // basic idea - iterate through incoming connections, computing inputs.  These get
        // summed and processed through a transfer function to generate an reactivity between 
        // [-1,1].  Note that edge transEff and influence have already been set for this cycle.

        if (bias === null)
            bias = 0;

        // Step one - aggregate input signals
        var incomers = node.incomers('edge');  // Note - incomers are nodes, not edges
        var sumInfs = 0;

        for (var i = 0; i < incomers.length; i++) {
            let a = incomers[i].data('name')
            let k = incomers[i].data('influence')
            console.log(a, " - ", k)
            sumInfs += incomers[i].data('influence');
        }

        console.log(node.data('name'), " suminfsTotal :", sumInfs)

        // srTotal is the total aggregated input signal (influence) this actor is experiencing.
        var srTotal = this.AggregateInputFn(sumInfs + bias);

        console.log(node.data('name')," srTotal :",srTotal )

        // apply signal degradation factor in incoming influence. NOTE - this was moved to happen when activation occurs
        //srTotal = srTotal * (1 - kD);

        var reactivity = this.ActivationFn(srTotal, node.data('reactivityMovAvg'));

        if (isNaN(reactivity)) {
            this.ErrorMsg("Bad Reactivity calculation for node " + node.data('name') + ": srTotal=" + srTotal + ", sumInfs=" + sumInfs, ", movAvg=" + node.data('reactivityMovAvg'));
            reactivity = 0;
        }

        node.data('sumInfs', sumInfs);
        node.data('srTotal', srTotal);
        node.data('reactivity', reactivity);    // update store to new activation level

        //console.log('Activating node ' + node.data('id') + ', SumInfs=' + sumInfs.toFixed(2) + ', srTotal=' + srTotal.toFixed(2) + ', Reactivity=' + reactivity.toFixed(2))

        return reactivity;  // [-1,1]
    }

    ProcessLandscapeSignal(inputLevel, landscapeTraits, nodeTraits) {
        return inputLevel;
    }

    AggregateInputFn(sumInputSignals) {
        // normalize input to [0,transEffMax]
        // larger values result in a steeper curve
        // scale input signals to 0-transEffMax, based on the absolute value of the signal
        var signal = 0;
        if (sumInputSignals > 0)
            signal = 1 - Math.pow(this.aggInputSigma, -sumInputSignals);
        else if (sumInputSignals < 0)
            signal = -1 + Math.pow(this.aggInputSigma, sumInputSignals);

        return signal * this.transEffMax;  // srTotal
    }

    ActivationFn(inputSignal, reactivityMovAvg) {
        // transform normalized input {-1,1] into reactivity using activation function
        var reactivity = 0;
        if (Math.abs(inputSignal) > this.activationThreshold) {
            //const scale = 2.0;
            //const shift = 1.0;
            var b = this.activationSteepFactB - this.kF * reactivityMovAvg;

            reactivity = (2 * (1 / (1 + Math.exp(-b * inputSignal)))) - 1;
        }

        return reactivity;
    }

    GetOutputLevel() {
        // output level is defined as the average activation level of all the nodes
        // may add alternatives later!!
        let output = 0;
        let count = 0;
        this.cy.nodes(NT_LANDSCAPE_ACTOR).forEach(function (node) {
            output += node.data('reactivity');
            count++;
        });

        return output / count;
    }

    GetInputLevel(cycle) {  // note: cycle is ZERO based
        switch (this.inputSeriesType) {
            case 1:  // constant
            case 2:  // constant with stop
            case 3:  // sinesoidal
            case 4:  // random
                return this.netInputs[cycle];

            case 5: { // track output
                //let lagPeriod = parseInt($("#lagPeriod").val());

                // are we at the start of a simulation
                if (cycle <= this.lagPeriod) {
                    return this.initialValue; //parseFloat($("#initialValue").val());
                }
                else {
                    //var slope = parseFloat($("#slope").val());

                    var output = 0;
                    if (this.lagPeriod === 0)
                        output = GetOutputLevel(this);
                    else
                        output = this.netOutputs[cycle - this.lagPeriod];

                    var input = this.slope * output;
                    this.netInputs[cycle] = input;
                    return input;
                }
            }
        }
    }

    UpdateNetworkStats() {
        if (this.cy === null)
            return;

        var edgeCount = this.cy.edges(NETWORK_EDGE).length;
        var nodeCount = this.cy.nodes(ANY_ACTOR).length;
        var localClusterCoeff = 0;

        var cycle = this.currentCycle < 0 ? 0 : this.currentCycle;
        var inputActivation = this.GetInputLevel(cycle);

        this.netStats = {
            // simulation stats
            cycle: cycle,
            convergeIterations: this.convergeIterations,

            // basic network info
            nodeCount: nodeCount,
            edgeCount: edgeCount,
            nodeCountNLA: 0.0,
            minNodeReactivity: 1.0,
            meanNodeReactivity: 0.0,
            maxNodeReactivity: 0.0,
            minNodeInfluence: 1.0,
            meanNodeInfluence: 0.0,
            maxNodeInfluence: 0.0,
            minEdgeTransEff: 1.0,
            meanEdgeTransEff: 0.0,
            maxEdgeTransEff: 0.0,
            minEdgeInfluence: 1.0,
            meanEdgeInfluence: 0.0,
            maxEdgeInfluence: 0.0,
            edgeDensity: 0.0,
            totalEdgeSignalStrength: 0.0,

            // input/output
            inputActivation: inputActivation,
            landscapeSignalInfluence: 0.0,
            outputActivation: this.GetOutputLevel(),
            meanLANodeReactivity: 0.0,
            maxLANodeReactivity: 0.0,
            signalContestedness: 0.0,

            // network
            meanDegreeCentrality: 0.0,
            meanBetweenessCentrality: 0.0,
            meanClosenessCentrality: 0.0,
            meanInfWtDegreeCentrality: 0.0,
            meanInfWtBetweenessCentrality: 0.0,
            meanInfWtClosenessCentrality: 0.0,

            normCoordinators: 0.0,
            normGatekeeper: 0.0,
            normRepresentative: 0.0,
            notLeaders: 0.0,
            clusteringCoefficient: localClusterCoeff,
            NLA_Influence: 0.0,
            NLA_TransEff: 0.0,
            normNLA_Influence: 0.0,
            normNLA_TransEff: 0.0,
            normEdgeSignalStrength: 0.0
        };

        let landscapeActorCount = 0;
        let inputSignalCount = 0;
        let _this = this;
        this.cy.nodes(ANY_ACTOR).forEach(function (node) {
            // min reactivity
            var nodeReactivity = node.data('reactivity');
            if (nodeReactivity < _this.netStats.minNodeReactivity)
                _this.netStats.minNodeReactivity = nodeReactivity;
            // max reactivity
            if (nodeReactivity > _this.netStats.maxNodeReactivity)
                _this.netStats.maxNodeReactivity = nodeReactivity;
            // mean reactivity
            _this.netStats.meanNodeReactivity += nodeReactivity;

            if (node.data('type') === NT_LANDSCAPE_ACTOR) {
                _this.netStats.meanLANodeReactivity += nodeReactivity;
                landscapeActorCount += 1;
            }

            else if (node.data('type') === NT_INPUT_SIGNAL) {
                inputSignalCount += 1;
            }
        });

        this.netStats.networkNodeCount = this.netStats.nodeCount - (landscapeActorCount + inputSignalCount)

        this.netStats.meanNodeReactivity /= nodeCount;

        if (landscapeActorCount > 0)
            this.netStats.meanLANodeReactivity /= landscapeActorCount;

        this.cy.nodes(ANY_ACTOR).forEach(function (node) {
            var nodeInfluence = node.data('influence');
            // min reactivity
            if (nodeInfluence < _this.netStats.minNodeInfluence)
                _this.netStats.minNodeInfluence = nodeInfluence;
            // max reactivity
            if (nodeInfluence > _this.netStats.maxNodeInfluence)
                _this.netStats.maxNodeInfluence = nodeInfluence;
            // mean reactivity
            _this.netStats.meanNodeInfluence += nodeInfluence;
        });

        this.netStats.meanNodeInfluence /= nodeCount;

        this.cy.nodes(INPUT_SIGNAL).forEach(function (node) {
            _this.netStats.landscapeSignalInfluence += node.data('influence');
        });

        // next, do edges
        this.cy.edges(NETWORK_EDGE).forEach(function (edge) {
            // transmission efficiency
            var transEff = edge.data('transEff');
            if (transEff < _this.netStats.minEdgeTransEff)
                _this.netStats.minEdgeTransEff = transEff;

            if (transEff > _this.netStats.maxEdgeTransEff)
                _this.netStats.maxEdgeTransEff = transEff;

            _this.netStats.meanEdgeTransEff += transEff;

            // influence
            var influence = edge.data('influence');

            if (influence < _this.netStats.minEdgeInfluence)
                _this.netStats.minEdgeInfluence = influence;

            if (influence > _this.netStats.maxEdgeInfluence)
                _this.netStats.maxEdgeInfluence = influence;

            _this.netStats.meanEdgeInfluence += influence;
        });

        this.netStats.meanEdgeTransEff /= edgeCount;
        this.netStats.meanEdgeInfluence /= edgeCount;

        // LAReactivity.reduce((a, b) => (a + b)) / LAReactivity.length
        // traditional SA stats
        let cs = this.GetDegreeCentralities(NETWORK_ACTOR, true);        
        this.netStats.meanDegreeCentrality = cs.length > 0 ? cs.reduce((a, b) => (a + b)) / cs.length : 0;

        cs = this.GetInfWtDegreeCentralities(NETWORK_ACTOR, true);
        this.netStats.meanInfWtDegreeCentrality = cs.length > 0 ? cs.reduce((a, b) => (a + b)) / cs.length : 0;

        cs = this.GetBetweennessCentralities(NETWORK_ACTOR, true);
        this.netStats.meanBetweennessCentrality = cs.length > 0 ? cs.reduce((a, b) => (a + b)) / cs.length : 0;

        cs = this.GetInfWtBetweennessCentralities(NETWORK_ACTOR, true);
        this.netStats.meanInfWtBetweenCentrality = cs.length > 0 ? cs.reduce((a, b) => (a + b)) / cs.length : 0;

        cs = this.GetClosenessCentralities(NETWORK_ACTOR, true);
        this.netStats.meanClosenessnessCentrality = cs.length > 0 ? cs.reduce((a, b) => (a + b)) / cs.length : 0;

        cs = this.GetInfWtClosenessCentralities(NETWORK_ACTOR, true);
        this.netStats.meanInfWtClosenessnessCentrality = cs.length > 0 ? cs.reduce((a, b) => (a + b)) / cs.length : 0;

        // Generate final values for netStats

        // motif representation
        let coordinators = this.GetMotifs(MOTIF_COORDINATOR);
        let gatekeepers = this.GetMotifs(MOTIF_GATEKEEPER);
        let representatives = this.GetMotifs(MOTIF_REPRESENTATIVE);
        let leaders = this.GetMotifs(MOTIF_LEADER);
        this.netStats.normCoordinators = coordinators.length / this.netStats.nodeCount;
        this.netStats.normGatekeeper = gatekeepers.length / this.netStats.nodeCount;
        this.netStats.normRepresentative = representatives.length / this.netStats.nodeCount;
        this.netStats.normLeaders = leaders.length / this.netStats.nodeCount;

        // ???
        var edgeCountNLA = 0;
        var edgeCountLSN = 0;
        var edgeCountN = 0;
        var NLA_Influence = 0;
        var NLA_TransEff = 0;
        var totalEdgeSignalStrength = 0;
        var lanc = 0;
        var contestt = 0;

        this.cy.edges().forEach(function (edge) {
            if (edge.target().data('type') === NT_LANDSCAPE_ACTOR) {   // is target of edge a landscape actor?
                edgeCountNLA = edgeCountNLA + 1;
                NLA_Influence = NLA_Influence + edge.data('influence');
                NLA_TransEff = NLA_TransEff + edge.data('transEff');
            }
            else if (edge.source().data('type') === NT_INPUT_SIGNAL) {
                edgeCountLSN += 1;
                totalEdgeSignalStrength += edge.data('signalStrength');
            }

            else {
                edgeCountN += 1;
                totalEdgeSignalStrength += edge.data('signalStrength');
            }
        });

        //Contestedness
        //Add the sign of the sender node to the influence. A contested signal will have a contestt value of 0
        // 
        this.cy.nodes(LANDSCAPE_ACTOR).forEach(function (node) {
            lanc += 1
            let cont_temp = 0
            let max_inf = 0
            node.incomers('edge').forEach(function (edge) {
                let sn = edge.data('source');
                let nodetraits = _this.cy.elements('node#' + sn).data('traits');
                cont_temp += edge.data("influence") * (nodetraits / Math.abs(nodetraits))

                //Possibly add a maximum possible influence value here. If cont_temp at the end is greater than any max influence, then the signal is past the contested threshold
                //But we might also not need that as contestedness requires opposite signals of the same magnitude. So as we move farther away from 0 we have
                // a less contested signal, it's just higher values mean that the signal is uncontested at a greater level.
                if (edge.data("influence") > max_inf) {
                    max_inf = edge.data("influence");
                };

            });

            
            contestt += Math.abs(cont_temp)
            
        });

        this.netStats.signalContestedness = contestt / lanc;

        this.netStats.normNLA_Influence = NLA_Influence / edgeCountNLA;
        this.netStats.normNLA_TransEff = NLA_TransEff / edgeCountNLA;

        this.netStats.normEdgeSignalStrength = totalEdgeSignalStrength / this.netStats.edgeCount;

        // Measure of Edge Density w/r/t Landscape Signals and Landscape Actors
        // Maximum possible edges for LSN == Network Actors, NLA == Network Actors * # Landscape Actors

        this.netStats.edgeMakeupLSN = edgeCountLSN / this.netStats.networkNodeCount
        this.netStats.edgeMakeupNLA = edgeCountNLA / (this.netStats.networkNodeCount * lanc)
        // Collaboration Metric Currently the Density of the inner network (innie-innie network??)? 
        this.netStats.networkActorDensity = edgeCountN / (this.netStats.networkNodeCount * (this.netStats.networkNodeCount - 1));

        //console.log(this.cy.$().betweennessCentrality({ directed: true }));
        var LAReactivity = []
        
        this.cy.nodes(ANY_ACTOR).forEach(function (node) {
            if (node.data('type') === NT_LANDSCAPE_ACTOR) {;
                LAReactivity.push(node.data("reactivity"));
            }
            if (node.data('type') === NT_INPUT_SIGNAL) {
                landscapeSignalInfluence = node.data("influence");
            }
        });



        this.landscapeSignalInfluence = this.landscapeSignalInfluence
        this.netStats.maxLANodeReactivity = Math.max(...LAReactivity);
        this.netStats.meanLANodeReactivity = LAReactivity.reduce((a, b) => (a + b)) / LAReactivity.length;

        return this.netStats;
    }

    
    // create an array with the count of nodes for each allowed degree bin
    BuildNodeDistributionArrays() {
        if (this.cy === null)
            return;

        let _this = this;

        // make an array, including a slot for '0'
        this.degreeDistributions = Array(this.maxNodeDegree + 1).fill(0);  // add one for full range
        this.reactivityDistributions = [];
        let rxBinWidth = 2 / 20;
        for (var i = 0; i < 20; i++) {
            let binVal = -1 + i * rxBinWidth + rxBinWidth / 2;
            this.reactivityDistributions.push([binVal, 0]);
        }

        let infBinWidth = this.maxNodeInfluence / 20;
        this.influenceDistributions = [];
        for (i = 0; i < 20; i++) {
            let binVal = i * infBinWidth + infBinWidth / 2;
            this.influenceDistributions.push([binVal, 0]);
        }

        // get the node degree stored with the node
        this.cy.nodes(ANY_ACTOR).forEach(function (node) {
            var degree = node.degree(false);
            if (degree >= _this.degreeDistributions.length)
                this.ErrorMsg("BuildNodeDistributionArrays() error",
                    "bad degree encountered! (" + degree + " found, max is " + _this.maxNodeDegree + ")",
                    false);
            else
                _this.degreeDistributions[degree] += 1;

            var reactivity = node.data('reactivity');
            var bin = Classify(reactivity, -1, 1, 20);
            _this.reactivityDistributions[bin][1] += 1;

            // build influence distribution array for this node
            var influence = 0;
            node.outgoers('edge').forEach((edge) => { influence += edge.data('influence'); });
            bin = Classify(influence, 0, _this.maxNodeInfluence, 20);
            _this.influenceDistributions[bin][1] += 1;
        });

        // build influence distrubtion array as well
        //network.nodes(ANY_ACTOR).forEach(function (node) {
        //    var influence = 0;
        //    node.outgoers('edge').forEach((edge) => { influence += edge.data('influence'); });
        //    var bin = Classify(influence, 0, maxNodeInfluence, 20);
        //    influenceDistributions[bin] += 1;
        //});
    }

    // create an array with the count of nodes for each allowed degree bin
    BuildEdgeDistributionArrays() {
        if (this.cy === null)
            return;

        // make an array, including a slot for '0'
        this.transEffDistributions = Array(20).fill(0);  // add one for full range
        this.signalStrengthDistributions = Array(20).fill(0);  // add one for full range
        let _this = this;

        // get the node degree stored with the node
        this.cy.edges(NETWORK_EDGE).forEach(function (edge) {
            var transEff = edge.data('transEff');
            var bin = Classify(transEff, 0, _this.transEffMax, 20);
            _this.transEffDistributions[bin] += 1;

            var sigStr = edge.data('signalStrength');
            bin = Classify(sigStr, 0, 1, 20);
            _this.signalStrengthDistributions[bin] += 1;

            //var influence = edge.data('influence');
            //bin = Classify(influence, -1,1, 20);
            //_this.influenceDistributions[bin] += 1;
        });

    }


    // public methods

    static BuildNetwork(divNetwork, networkInfo) {
        // make a default network object
        let snipModel = new SnipModel();  // create new SnipModel object

        //Node.js Testing
        if (divNetwork == "Test"){
            snipModel.randGenerator = Math.random();
        }
        else{
            snipModel.randGenerator = new Math.seedrandom(networkInfo.name);
        }


        snipModel.networkInfo = networkInfo;


        if ("settings" in networkInfo)
            snipModel.LoadPreBuildSettings(networkInfo.settings);

        let nodes = snipModel.BuildNodes(networkInfo.nodes, networkInfo.traits);
        let edges = snipModel.BuildEdges(networkInfo.edges, nodes);

        // instantiate a cytocspe network object
        snipModel.cy = cytoscape({
            container: $('#' + divNetwork),
            minZoom: 0.2,
            maxZoom: 4,
            elements: {
                nodes: nodes,
                edges: edges
            },
            wheelSensitivity: 0.05,
            style: SnipModel.GetNetworkStyleSheet(),
            layout: snipModel.layout
        });

        snipModel.inputNode = snipModel.cy.nodes(INPUT_SIGNAL)[0];
        //InitSimulation();

        // setup up initial network configuration
        snipModel.AddAutogenInputEdges();
        snipModel.SetEdgeTransitTimes();

        snipModel.ResetNetwork();
        snipModel.GenerateInputArray();
        snipModel.SetInputNodeReactivity(snipModel.netInputs[0]);
        snipModel.maxNodeDegree = snipModel.cy.nodes().maxDegree(false);

        snipModel.SolveEqNetwork(0);

        //snipModel.InitSimulation();

        snipModel.UpdateNetworkStats();

        if ("settings" in networkInfo)
            snipModel.LoadPostBuildSettings(networkInfo.settings);

        snipModel.cy.on('mouseover', 'node', ShowNodeTip);
        snipModel.cy.on('mouseout', 'node', HideTip);
        snipModel.cy.on('mouseover', 'edge', ShowEdgeTip);
        snipModel.cy.on('mouseout', 'edge', HideTip);

        return snipModel;
    }

    RunSimulation(initialize, step) {

        if (initialize)
            this.InitSimulation();

        // set up progress bar
        if (this.currentCycle === 0) {
            var $divProgressBar = $('#divProgressBar');
            if (divProgressBar) {
                $divProgressBar.progress('set active');
                $divProgressBar.progress('reset');
                $divProgressBar.progress('set total', this.cycles);
                $('#divProgressText').text("1/" + this.cycles);
                $('#divProgress').show();
            }

            //this.UpdateWatchLists();

            // set up array to store outputs
            this.netOutputs = Array(this.cycles).fill(0);

            this.UpdateNetworkStats();
            this.netReport.push(this.netStats);

            ///?????ResetOutputCharts();
        }

        // if "step" model, call RunCycle() with repeatUntilDone=false; otherwise, true
        // Note that RunCycle will ansynchronously recurse after this function completes
        // and that end-of-simulation function must therefore be taken care of in RunCycle()
        SnipModel.RunCycle(this, this.currentCycle, !step);
    }

    SolveEqNetwork(bias) { // selector = ACTIVE; ANY_ACTOR
        // Solves the network to steady state.  starting condition is whatever state the 
        // network is currently in - this function does not do the initialization.
        if (this.cy === null)
            return;

        if (bias === null)
            bias = 0;

        // set input nodes to their appropriate values
        let stepDelta = 9999999.0;
        let nodeCount = this.cy.nodes(ACTIVE).length;
        let deltaTolerance = 0.0001 * (nodeCount - 1);  // -1 since last node is landsacep signal
        const maxIterations = 400;
        let _this = this;

        // update the influence transmission efficiencies for each edge.  Not that this are independent of reactivities
        this.ComputeEdgeTransEffs(ACTIVE);

        // iteratively solve the network but letting it relax to steady state.
        this.convergeIterations = 0;

        while (stepDelta > deltaTolerance && this.convergeIterations < maxIterations) {


            // update all edges so that signal strength, used in the singal-reviever calculatons are equal
            this.cy.edges().forEach(function (edge) {
                // set signal strength for the activating edge based on upstream signal minus degradation
                // signal strength is based on the reactivity of the upstream node
                let sourceNode = edge.source();
                let srcReactivity = sourceNode.data('reactivity');    // what if it's an input signal?
                edge.data('signalStrength', srcReactivity * (1 - _this.kD));    // confirm for input signals!!!!
                edge.data('signalTraits', _this.inputNode.data('traits'));   // establish edge signal traits ADD INPUT NODE
            });



            //WatchMsg(null, "Run:" + runCount + ", Iteration:" + convergeIterations);
            // iterate though each node (excluding input), calculating an activation for that node
            // we will repeat this until the accumulate delta goes below a threshold value.
            this.ComputeEdgeInfluences(ACTIVE);  // note that these are dependent on reactivitities



            stepDelta = 0.0;
            this.cy.nodes(ACTIVE).forEach(function (node) {
                if (node.data('type') !== NT_INPUT_SIGNAL) {  // don't compute reactivities for input nodes 
                    var oldReactivity = node.data('reactivity');   //  [-1,1]
                    var newReactivity = _this.ActivateNode(node, bias); // this updates the node's 'reactivity', 'sumInf', 'srTotal' data
                    var delta = newReactivity - oldReactivity;

                    console.log(node.data('name'), " old: ", oldReactivity, " new: ", newReactivity)
                    stepDelta += delta * delta;   // change from last cycle?
                }
            });

            // update reactivities
            //cy.nodes(selector).forEach(function (node) {
            //    var newReactivity = node.data('updatedReactivity');
            //    // apply decay
            //    //newReactivity *= (1 - reactivityDecayRate);
            //    node.data('reactivity', newReactivity);
            //});



            //WatchMsg(null, "--Delta this iteration: " + stepDelta.toPrecision(3)); //Fixed(5));
            this.convergeIterations++;
        }   // end of: while(network not converged)

        // update node influence levels
        this.maxNodeInfluence = 0.1;
        this.cy.nodes(ACTIVE).forEach(function (node) {
            var influence = 0;
            node.outgoers('edge').forEach(function (edge) { influence += edge.data('influence'); });
            node.data('influence', influence);
            if (influence > _this.maxNodeInfluence ) ////// && node.data('type') !== 0)  // exclude signals
                _this.maxNodeInfluence = influence;
        });

        // report any watch outputs
        if (this.OnWatchMsg !== null) {
            this.cy.nodes('[?watch]').forEach(function (node) {
                _this.OnWatchMsg(node, "--Node " + node.data('id') + ": sumInfs=" + node.data('sumInfs').toFixed(3) +
                    ",  srTotal=" + node.data('srTotal').toFixed(3) + ", reactivity=" + newReactivity.toFixed(3) + ", delta=" + delta.toFixed(3));
            });

            this.cy.edges('[?watch]').forEach(function (edge) {
                this.OnWatchMsg(edge, "--Edge " + edge.data('id') + ": transEff=" + edge.data('transEff').toFixed(3)
                    + ", transEffSender=" + edge.data('transEffSender').toFixed(3)
                    + ", transEffSignal=" + edge.data('transEffSignal').toFixed(3)
                    + ", influence=" + edge.data('transEffSignal').toFixed(3)
                    + ", signalStrength=" + edge.data('signalStrength').toFixed(3));

            });
        }

        console.log("Show StepDelta: ", stepDelta)

        return stepDelta;
    }

    ////function ApplyLearningRule(this) {
    ////    // this learning rule examines pairs of connected nodes, and increases 
    ////    // the connection strength if the pair activates at similar levels,
    ////    // and reduce the connection strength if the pair is unaligned.  To measure
    ////    // alignment, compute the magnitude of the vector connecting them
    ////    for (var from = 0; from < nodeCount; from++) {
    ////        for (var to = 0; to < nodeCount; to++) {
    ////            if (Math.abs(adjMatrix[from][to]) > closeToZero) {
    ////                // get node activations if connected
    ////                var fromNodeActivation = nodeInfos[from].activation;
    ////                var toNodeActivation = nodeInfos[to].activation;
    ////
    ////                // because the activation space is one dimensional, the delta gives us the
    ////                // similarity measure; values close to zero indicate close alignment,
    ////                // values close 2 (the max distance) indicate disalignment
    ////                var nodeDelta = Math.abs(fromNodeActivation - toNodeActivation);
    ////
    ////                // adjust edge weight proportionally to the alignment if 
    ////
    ////                var edgeDelta = GetEdgeDeltaFromNodeDelta(nodeDelta);
    ////
    ////                adjMatrix[from][to] += edgeDelta;
    ////            }
    ////        }
    ////    }
    ////}

    GenerateNetworkRep(includePos) {
        // construct a dictionary for the model representation
        let input = '"input": {\n\t\t\t\t"type": "' + this.inputType + '"';

        switch (this.inputType) {
            case "constant":
                input += ',\n\t\t\t\t"value": ' + this.initialValue;
                break;
            case "constant_with_stop":
                input += ',\n\t\t\t\t"value": ' + this.k1;
                input += ',\n\t\t\t\t"stop": ' + this.stop;
                break;
            case "sinusoidal":
                input += ',\n\t\t\t\t"amplitude": ' + this.amp;
                input += ',\n\t\t\t\t"period": ' + this.period;
                input += ',\n\t\t\t\t"phase_shift": ' + this.phase;
                break;
            case "random":
                break;
            case "track_output":
                break;
        }
        input += '\n\t\t\t},';

        let layoutName = this.layout.name;
        if (includePos)
            layoutName = 'preset';

        let sInfSubmodel = "sender_receiver";
        if (this.infSubmodel === IM_SIGNAL_RECEIVER)
            sInfSubmodel = "signal_receiver";
        else if (this.infSubmodel === IM_SIGNAL_SENDER_RECEIVER)
            sInfSubmodel = "signal_sender_receiver";

        let str = '{\n\t"network": {\n\t\t"name": "' + this.networkInfo.name + '",\n\t\t"description": "' + this.networkInfo.description + '",'
            + '\n\t\t"traits": ' + JSON.stringify(this.networkInfo.traits) + ','
            + '\n\t\t"settings": {'

            + '\n\t\t\t"autogenerate_landscape_signals" : {'
            + '\n\t\t\t\t"fraction": ' + this.autogenFraction + ','
            + '\n\t\t\t\t"bias": "' + this.autogenBias + '"\n\t\t\t},'

            + '\n\t\t\t"autogenerate_transit_times" : {'
            + '\n\t\t\t\t"max_transit_time": ' + this.autogenTransTimeMax + ','
            + '\n\t\t\t\t"bias": "' + this.autogenTransTimeBias + '"\n\t\t\t},'

            + '\n\t\t\t' + input
            + '\n\t\t\t"infSubmodel": "' + sInfSubmodel + '",'
            + '\n\t\t\t"infSubmodelWt": ' + this.infSubmodelWt + ','
            + '\n\t\t\t"agg_input_sigma": ' + this.aggInputSigma + ','
            + '\n\t\t\t"trans_eff_max": ' + this.transEffMax + ','
            + '\n\t\t\t"reactivity_threshold": ' + this.activationThreshold + ','
            + '\n\t\t\t"reactivity_steepness_factor_B": ' + this.activationSteepFactB + ','
            + '\n\t\t\t"node_size": "' + this.nodeSize + ','
            + '\n\t\t\t"node_color": "' + this.nodeColor + ','
            + '\n\t\t\t"node_label": "' + this.nodeLabel + ','
            + '\n\t\t\t"edge_size": "' + this.edgeSize + ','
            + '\n\t\t\t"edge_color": "' + this.edgeColor + ','
            + '\n\t\t\t"edge_label": "' + this.edgeLabel + ','
            + '\n\t\t\t"show_tips": ' + this.showTips + ','
            + '\n\t\t\t"show_landscape_signal_edges": ' + this.showLandscapeSignalEdges + ','
            + '\n\t\t\t"layout": "' + this.layoutName + '",'
            + '\n\t\t\t"zoom": ' + this.cy.zoom().toFixed(3) + ","
            + '\n\t\t\t"center": ' + JSON.stringify(this.cy.pan()) + ","
            + '\n\t\t\t"simulation_period": ' + this.cycles
            + '\n\t\t},';


        let nodes = '\n\t"nodes": [';
        let _this = this;

        this.cy.nodes().forEach(function (node, index) {
            let nodeType = '';
            switch (node.data("type")) {
                case NT_INPUT_SIGNAL: nodeType = 'input'; break;
                case NT_NETWORK_ACTOR: nodeType = 'network'; break;
                case NT_LANDSCAPE_ACTOR: nodeType = 'landscape'; break;
            }

            let pos = '';
            if (includePos) {
                pos = ',\n\t\t\t"x": ' + node.position("x").toFixed(3)
                    + ',\n\t\t\t"y": ' + node.position("y").toFixed(3);
            }
            nodes += '\n\t\t{\n\t\t\t"name": "' + node.data("name") + '",'
                + '\n\t\t\t"type": "' + nodeType + '",'
                + '\n\t\t\t"traits": ' + JSON.stringify(node.data('traits'))
                + pos
                + '\n\t\t}';

            if (index < _this.cy.nodes().length - 1)
                nodes += ',';
        });
        nodes += '\n\t],';

        let edges = '\n\t"edges": [';
        this.cy.edges().forEach(function (edge, index) {
            edges += '\n\t\t {\n\t\t\t"from": "' + edge.source().data("name") + '",'
                + '\n\t\t\t"to": "' + edge.target().data("name") + '"'
                + '\n\t\t}';

            if (index < _this.cy.edges().length - 1)
                edges += ',';
        });
        edges += '\n\t]';

        str += nodes + edges + '\n\t}\n}';

        return str;
    }

    UpdateLayoutOptions() {
        // add trait data to network view dialog
        var nodeSize = $("#ddlLayoutNodeSize");
        nodeSize.empty();
        nodeSize.append($('<option>').val("none").text("No Selection"));
        nodeSize.append($('<option selected>').val("degree").text("Degree [0,n]"));
        nodeSize.append($('<option>').val("reactivity").text("Reactivity [-1,1]"));
        nodeSize.append($('<option>').val("influence").text("Influence [-1,1]"));

        for (var i = 0; i < this.networkInfo.traits.length; i++) {
            let trait = this.networkInfo.traits[i];
            nodeSize.append($('<option>').val(trait).text(trait));
        }

        var nodeColor = $("#ddlLayoutNodeColor");
        nodeColor.empty();
        nodeColor.append($('<option>').val("none").text("No Selection"));
        nodeColor.append($('<option>').val("degree").text("Degree [0,n]"));
        nodeColor.append($('<option selected>').val("reactivity").text("Reactivity [-1,1]"));
        nodeColor.append($('<option>').val("influence").text("Influence [-1,1]"));

        for (i = 0; i < this.networkInfo.traits.length; i++) {
            let trait = this.networkInfo.traits[i];
            nodeColor.append($('<option>').val(trait).text(trait));
        }

        // Node Labels
        var nodeLabel = $("#ddlLayoutNodeLabel");
        nodeLabel.empty();
        nodeLabel.append($('<option selected>').val("none").text("No Selection"));
        nodeLabel.append($('<option>').val("name").text("Name"));
        nodeLabel.append($('<option>').val("degree").text("Degree [0,n]"));
        nodeLabel.append($('<option>').val("reactivity").text("Reactivity [-1,1]"));
        nodeLabel.append($('<option>').val("influence").text("Influence [-1,1]"));

        for (i = 0; i < this.networkInfo.traits.length; i++) {
            let trait = this.networkInfo.traits[i];
            nodeLabel.append($('<option>').val(trait).text(trait));
        }

        // set current selection
        $('#ddlLayoutNodeSize').dropdown("set selected", this.nodeSize);
        $('#ddlLayoutNodeColor').dropdown("set selected", this.nodeColor);
        $('#ddlLayoutNodeLabel').dropdown("set selected", this.nodeLabel);
        $('#ddlLayoutEdgeWidth').dropdown("set selected", this.edgeSize);
        $('#ddlLayoutEdgeColor').dropdown("set selected", this.edgeColor);
        $('#ddlLayoutEdgeLabel').dropdown("set selected", this.edgeLabel);
    }

    SetNodeSizes() {
        if (this === null || this.cy === null)
            return;

        this.cy.startBatch();
        var ddlNS = document.getElementById('ddlLayoutNodeSize');
        let _this = this;

        var size = 24;
        if (ddlNS.value === 'degree')
            this.cy.nodes().forEach(function (node) {
                size = node.degree(false) / _this.maxNodeDegree; // [0,1]
                size = Math.round(size * 36);   // 6
                if (size < 6)
                    size = 6;
                node.data('size', size);
            });

        else if (ddlNS.value === 'reactivity')
            this.cy.nodes().forEach(function (node) {
                //size = (node.data('reactivity') + 1) * 0.5; // [-1,1] -> [0-1]
                size = Math.abs(node.data('reactivity'));// [-1,1] -> [0-1]
                size = Math.round(size * 42);
                if (size < 6)
                    size = 6;
                node.data('size', size);
            });

        else if (ddlNS.value === 'influence')
            this.cy.nodes().forEach(function (node) {
                size = (node.data('influence') / _this.maxNodeInfluence); // [-1,1] -> [0-1]
                size = Math.abs(size);
                size = Math.round(size * 42);
                if (size < 6)
                    size = 6;
                node.data('size', size);
            });

        else {  // its a trait
            this.cy.nodes().forEach(function (node) {
                let traitIndex = 0;
                for (traitIndex = 0; traitIndex < _this.networkInfo.traits.length; traitIndex++)
                    if (ddlNS.value === _this.networkInfo.traits[traitIndex])
                        break;

                var traits = node.data('traits');
                if (traits === null)
                    ;
                else {
                    var traitValue = traits[traitIndex];
                    if (isNaN(traitValue))   // not a number?
                        size = 24;
                    else {
                        size = (traitValue + 1) * 0.5; // [-1,1] -> [0-1]
                        size = Math.round(size * 24);
                    }
                }
                if (size < 6)
                    size = 6;
                node.data('size', size);
            });
        }
        this.cy.endBatch();
    }

    SetNodeColors() {
        if (this === null || this.cy === null)
            return;

        this.cy.startBatch();
        let ddlNC = document.getElementById('ddlLayoutNodeColor');
        let _this = this;
        //if (src === 1)
        //    ddlNC.value = document.getElementById('ddlLayoutNodeColorLegend').value;

        if (ddlNC.value === 'degree')
            this.cy.nodes().forEach(function (node) {
                var degree = node.degree(false);
                var color = MapColor(degree, 0, _this.maxNodeDegree, redGreenColorRamp);
                node.data('backgroundColor', color);
            });

        else if (ddlNC.value === 'reactivity')
            this.cy.nodes().forEach(function (node) {
                var reactivity = node.data('reactivity');
                var color = MapColor(reactivity, -1, 1, redGreenColorRamp);
                node.data('backgroundColor', color);
            });

        else if (ddlNC.value === 'influence')
            this.cy.nodes().forEach(function (node) {
                var influence = node.data('influence');
                var color = MapColor(influence, 0, _this.maxNodeInfluence, redGreenColorRamp);
                node.data('backgroundColor', color);
            });


        else if (ddlNC.value === 'none')
            this.cy.nodes().forEach(function (node) {
                node.data('backgroundColor', 'green');
            });

        else {  // its a trait
            this.cy.nodes().forEach(function (node) {
                let traitIndex = 0;
                for (traitIndex = 0; traitIndex < _this.networkInfo.traits.length; traitIndex++)
                    if (ddlNC.value === _this.networkInfo.traits[traitIndex])
                        break;

                var traits = node.data('traits');
                if (traits === null)
                    ;
                else {
                    var traitValue = traits[traitIndex];
                    var color = MapColor(traitValue, -1, 1, redGreenColorRamp);
                    node.data('backgroundColor', color);
                }
            });
        }
        this.cy.endBatch();
    }

    SetNodeLabels() {
        if (this === null || this.cy === null)
            return;

        this.cy.startBatch();
        var ddlNL = document.getElementById('ddlLayoutNodeLabel');
        //if (src === 1)
        //    ddlNL.value = document.getElementById('ddlLayoutNodeLabelLegend').value;

        if (ddlNL.value === 'degree') {
            this.cy.nodes().forEach(function (node) {
                var degree = node.degree(false).toString();
                node.data('label', degree);
            });
            AddLabels(this, "node");
        }

        else if (ddlNL.value === 'reactivity') {
            this.cy.nodes().forEach(function (node) {
                var reactivity = node.data('reactivity').toFixed(2);
                node.data('label', reactivity);
            });
            this.AddLabels("node");
        }

        else if (ddlNL.value === 'influence') {
            this.cy.nodes().forEach(function (node) {
                var influence = node.data('influence').toFixed(2);
                node.data('label', influence);
            });
            this.AddLabels("node");
        }

        else if (ddlNL.value === 'name') {
            this.cy.nodes().forEach(function (node) {
                var name = node.data('name');
                node.data('label', name);
            });
            this.AddLabels("node");
        }

        else if (ddlNL.value === 'none') {
            this.cy.nodes().forEach(function (node) {
                node.data('label', '');
            });
            this.RemoveLabels("node");
        }

        else {  // its a trait
            let _this = this;
            this.cy.nodes().forEach(function (node) {
                let traitIndex = 0;
                for (traitIndex = 0; traitIndex < _this.networkInfo.traits.length; traitIndex++)
                    if (ddlNL.value === _this.networkInfo.traits[traitIndex])
                        break;
                var traits = node.data('traits');
                if (traits === null || traitIndex === _this.networkInfo.traits.length)
                    node.data('label', '');
                else {
                    var traitValue = traits[traitIndex];
                    node.data('label', traitValue.toString());
                }
            });
            this.AddLabels("node");
        }

        this.cy.endBatch();
    }

    SetEdgeWidths() {
        if (this === null || this.cy === null)
            return;

        this.cy.startBatch();
        let _this = this;

        // sets node 'size' param
        var ddlES = document.getElementById('ddlLayoutEdgeWidth');
        //if (src === 1)
        //    ddlES.value = document.getElementById('ddlLayoutEdgeWidthLegend').value;

        var width = 0.5;
        if (ddlES.value === 'influence')
            this.cy.edges().forEach(function (edge) {
                //width = (edge.data('influence') + 1) * 0.5; // [-1,1] -> [0-1]
                width = Math.abs(edge.data('influence')); // [-1,1] -> [0-1]
                width = Math.round(width * 8);
                if (width <= 0)
                    width = 0.1;
                edge.data('width', width);
            });

        else if (ddlES.value === 'transEff')
            this.cy.edges().forEach(function (edge) {
                width = edge.data('transEff') / _this.transEffMax; //  [0-1]
                width = Math.round(width * 8);
                if (width <= 0)
                    width = 0.1;
                edge.data('width', width);
            });

        else if (ddlES.value === 'transTime')
            this.cy.edges().forEach(function (edge) {
                let transTimeMax = _this.autogenTransTimeMax > 0 ? _this.autogenTransTimeMax : 1;
                width = edge.data('transTime') / transTimeMax; //  [0-1]
                width = Math.round(width * 8);
                if (width <= 0)
                    width = 0.1;
                edge.data('width', width);
            });

        else if (ddlES.value === 'signalStrength')
            this.cy.edges().forEach(function (edge) {
                //width = (edge.data('signalStrength') + 1) * 0.5; // [-1,1] -> [0-1]
                width = Math.abs(edge.data('signalStrength')); // [-1,1] -> [0-1]
                width = Math.round(width * 8);
                if (width <= 0)
                    width = 0.1;
                edge.data('width', width);
            });

        else if (ddlES.value === 'none')
            this.cy.edges().forEach(function (edge) {
                edge.data('width', width);
            });
        this.cy.endBatch();
    }

    SetEdgeColors() {
        if (this === null || this.cy === null)
            return;

        this.cy.startBatch();

        // sets node 'background' param
        var ddlEC = document.getElementById('ddlLayoutEdgeColor');
        let _this = this;

        //if (src === 1)
        //    ddlEC.value = document.getElementById('ddlLayoutEdgeColorLegend').value;

        if (ddlEC.value === 'transEff')
            this.cy.edges().forEach(function (edge) {
                var transEff = edge.data('transEff'); // [0, transEffMax]
                var color = MapColor(transEff, 0, _this.transEffMax, redGreenColorRamp);
                edge.data('lineColor', color);
            });

        else if (ddlEC.value === 'transTime')
            this.cy.edges().forEach(function (edge) {
                var transTime = edge.data('transTime'); // [0, transEffMax]
                let transTimeMax = _this.autogenTransTimeMax > 0 ? _this.autogenTransTimeMax : 1;
                var color = MapColor(transTime, 0, transTimeMax, redGreenColorRamp);
                edge.data('lineColor', color);
            });

        else if (ddlEC.value === 'influence')
            this.cy.edges().forEach(function (edge) {
                var influence = edge.data('influence');  // [-1,1]
                var color = MapColor(influence, -1, 1, redGreenColorRamp);
                edge.data('lineColor', color);
            });

        else if (ddlEC.value === 'signalStrength')
            this.cy.edges().forEach(function (edge) {
                var sigStr = (edge.data('signalStrength') + 1) * 0.5;  // [-1,1] => [0-1]
                var color = MapColor(sigStr, 0, 1, redGreenColorRamp);
                edge.data('lineColor', color);
            });

        else if (ddlEC.value === 'none')
            this.cy.edges().forEach(function (edge) {
                edge.data('lineColor', 'gray');
            });

        else
            ;

        this.cy.endBatch();
    }

    SetEdgeLabels() {
        if (this === null || this.cy === null)
            return;

        this.cy.startBatch();

        // sets node 'background' param
        var ddlEL = document.getElementById('ddlLayoutEdgeLabel');
        //if (src === 1)
        //   ddlEL.value = document.getElementById('ddlLayoutEdgeLabelLegend').value;

        if (ddlEL.value === 'transEff')
            this.cy.edges().forEach(function (edge) {
                var transEff = edge.data('transEff').toFixed(2); // [0, transEffMax]
                edge.data('label', transEff);
            });

        else if (ddlEL.value === 'transTime')
            this.cy.edges().forEach(function (edge) {
                let transTime = edge.data('transTime').toFixed(2);  // [-1,1]
                edge.data('label', transTime);
            });

        else if (ddlEL.value === 'influence')
            this.cy.edges().forEach(function (edge) {
                var influence = edge.data('influence').toFixed(2);  // [-1,1]
                edge.data('label', influence);
            });

        else if (ddlEL.value === 'signalStrength')
            this.cy.edges().forEach(function (edge) {
                var sigStr = edge.data('signalStrength').toFixed(2);  // [-1,1]
                edge.data('label', sigStr);
            });


        else if (ddlEL.value === 'toFrom')
            this.cy.edges().forEach(function (edge) {
                var source = edge.source();
                var target = edge.target();
                edge.data('label', source.data('name') + '->' + target.data('name'));
            });

        else if (ddlEL.value === 'none')
            this.cy.edges().forEach(function (edge) {
                edge.data('label', '');
            });

        else
            ;

        this.AddLabels("edge");
        this.cy.endBatch();
    }

    AddLabels(type) {
        if (this === null || this.cy === null)
            return;

        this.cy.style()
            .selector(type)
            .style({ 'label': 'data(label)' }) // what's this do?
            .update() // indicate the end of your new stylesheet so that it can be updated on elements
            ;
    }

    RemoveLabels(type) {
        if (this === null || this.cy === null)
            return;

        this.cy.style()
            .selector(type)
            .style({ 'label': '' })
            .update() // indicate the end of your new stylesheet so that it can be updated on elements
            ;
    }

    GetMotifs(motif) {

        // Gatekeeper - a gatekeeper is defined by:
        // receives an "out of group" incoming signal and propagates to at least
        // one "in group"
        let similarityThreshold = 0.5;
        let nodes = [];
        let _this = this;

        let bc = null;
        if (motif === MOTIF_LEADER)
            bc = this.cy.$().betweennessCentrality({ directed: true });

        this.cy.nodes(NETWORK_ACTOR).forEach(function (node) {
            let downstreamInGroup = false;
            let downstreamOutGroup = false;
            let upstreamOutGroup = false;
            let upstreamInGroup = false;
            let similarity = 0;

            node.neighborhood('edge').forEach(function (edge) {
                // downstream edge?
                if (edge.source() === node) {
                    similarity = _this.ComputeSimilarity(node.data('traits'), edge.target().data('traits'));
                    if (similarity > similarityThreshold)
                        downstreamInGroup = true;
                    else
                        downstreamOutGroup = true;

                } else { // upstream edge
                    similarity = _this.ComputeSimilarity(node.data('traits'), edge.source().data('traits'));
                    if (similarity < similarityThreshold)
                        upstreamOutGroup = true;
                    else
                        upstreamInGroup = true;
                }
            });

            switch (motif) {
                case MOTIF_GATEKEEPER:
                    if (downstreamInGroup && upstreamOutGroup)
                        nodes.push(node);
                    break;

                case MOTIF_COORDINATOR:
                    if (downstreamInGroup && upstreamInGroup)
                        nodes.push(node);
                    break;

                case MOTIF_REPRESENTATIVE:
                    if (downstreamOutGroup && upstreamInGroup)
                        nodes.push(node);
                    break;

                case MOTIF_LEADER:
                    const bcN = bc.betweennessNormalized('#' + node.data('id'));
                    if (bcN > 0.5) // ????
                        nodes.push(node);
                    break;
            }
        });

        return nodes;
    }


    GetDegreeCentralities(selector,normed) {
        let nas = this.cy.nodes(selector);  //NETWORK_ACTOR);
        let values = [];

        let maxDC = 0;
        if (normed) {
            nas.forEach(node => {
                let outgoers = node.outgoers('edge');
                if (outgoers.length > maxDC)
                    maxDC = outgoers.length;
            });
        }

        nas.forEach(node => {
            if (normed)
                values.push(node.outgoers('edge').length/maxDC);
            else
                values.push(node.outgoers('edge').length);
        });

        return values;
    }

    GetInfWtDegreeCentralities(selector, normed) {
        let nas = this.cy.nodes(selector);  //NETWORK_ACTOR);
        let values = [];

        let maxDC = 0;
        if (normed) {
            nas.forEach(node => {
                let outgoers = node.outgoers('edge');

                outgoers.forEach(edge => {
                    if (edge.data('influence') > maxDC)
                        maxDC = edge.data('influence');
                });
            });
        }

        nas.forEach(node => {
            let outgoers = node.outgoers('edge');
            let dci = 0;
            if (normed)
                outgoers.forEach(edge => dci += edge.data('influence') / maxDC);
            else
                outgoers.forEach(edge => dci += edge.data('influence'));

            values.push(dci);
        });

        return values;
    }


    GetBetweennessCentralities(selector, normed) {
        let nas = this.cy.nodes(selector);
        let bc = null;
        if (normed)     // NEEDS WORK!  NORM NOT IMPLEMTENTED
            bc = nas.bc({ directed: true });  // betweenness
        else
            bc = nas.bc({ directed: true });  // betweenness

        let values = [];
        let _this = this;
        nas.forEach(node => {
            let _bc = bc.betweenness(node);
            values.push(_bc);
        });

        return values;
    }

    GetInfWtBetweennessCentralities(selector, normed) {
        let nas = this.cy.nodes(selector);
        let bc = null;
        if (normed)
            bc = nas.bc({ weight: (edge) => edge.data('influence'), directed: true });  // betweenness
        else
            bc = nas.bc({ directed: true });  // betweenness

        let values = [];
        let _this = this;
        nas.forEach(node => {
            let _bc = bc.betweenness(node);
            values.push(_bc);
        });

        /*
        let ccn = cy.elements().dcn({ 'options.directed': true });
        $scope.ndd = cy.nodes().forEach(n => { n.data({ ccn: ccn.degree(n) }); })
        */
        return values;
    }

    GetClosenessCentralities(selector, normed) {
        let nas = this.cy.nodes(selector);
        let values = [];
        let cc = null;
        if (normed)
            cc = nas.ccn({ directed: true });
        else
            cc = nas.cc({ directed: true }); 

        let _this = this;
        nas.forEach(node => {
            let value = cc.closeness(node); 
            values.push(value);
        });

        return values;
    }

    GetInfWtClosenessCentralities(selector, normed) {
        let nas = this.cy.nodes(selector);
        let values = [];
        let cc = null;

        if (normed)
            cc = nas.ccn({ weight: (edge) => Math.abs(edge.data('influence')), directed: true });  // betweenness
        else
            cc = nas.cc({ directed: true });  // betweenness

        nas.forEach(node => {
            let value = cc.closeness(node);
            values.push(value);
        });

        return values;
    }


    static GetNetworkStyleSheet() {
        var netstyle = [ // the stylesheet for the graph
            //{
            //    selector: 'core',
            //    'background-color': 'black'
            //},
            {
                selector: 'node[show=1]',   // network node
                style: {
                    'display': 'element',
                    'width': function (ele) { return ele.data('size'); },
                    'height': function (ele) { return ele.data('size'); },
                    'opacity': 1, // function (ele) { return ele.data('state'); },
                    'background-color': function (ele) { return ele.data('backgroundColor'); },
                    'border-width': function (ele) { if (ele.data('watch') === true) return 3; else return 0; },
                    'border-style': 'solid',
                    'border-color': 'black',
                    'color': 'white',
                    //'background-image': 'data:image/svg+xml;utf8,' + encodeURIComponent('https://explorer.bee.oregonstate.edu/topic/InfluenceNetworks/Images/blank-eye-.svg'),
                    'label': ''
                }
            },
            {
                selector: 'node[show=1][type=0]',   // input signal nodes - highlight with blue border
                style: {
                    'border-width': 3,
                    'border-style': 'solid',
                    'border-color': 'blue'
                }
            },
            {
                selector: 'node[show=1][type=2]',   // landscape actor nodes - highlight with green border
                style: {
                    'border-width': 3,
                    'border-style': 'solid',
                    'border-color': 'brown'
                }
            },
            {
                selector: 'node[show=1][state=0]',   // inactive nodes - make semintransparent
                style: {
                    'opacity': 0.20
                }
            },
            {
                selector: 'node.flashing[show=1]',
                style: {
                    //'display': 'element',
                    'border-width': 10,
                    'border-style': 'dotted',
                    'border-color': 'yellow'
                }
            },
            {
                selector: 'node[show=0]',
                style: { 'display': 'none' }
            },
            {
                selector: 'edge[show=1]',
                style: {
                    'display': 'element',
                    'width': function (ele) { return ele.data('width'); },
                    'line-color': function (ele) { return ele.data('lineColor'); },
                    'target-arrow-color': '#ccc',
                    'target-arrow-shape': 'triangle',
                    'curve-style': 'bezier',
                    'control-point-step-size': 40,
                    'opacity': function (ele) { return ele.data('opacity'); },
                    'color': 'white',
                    'label': ''
                }
            },
            //{
            //    selector: 'edge[show=1][type=0]',   // lanscape signal edges
            //    style: {
            //        'width': 1,
            //        'opacity': function (ele) { return ele.data('opacity'); },
            //    }
            //},
            {
                selector: 'edge[show=1][state=0]',   // inactive nodes - make semintransparent
                style: {
                    'opacity': 0.20
                }
            },

            {
                selector: 'edge[show=1][state=3]',  // deleted
                style: { 'width': 0.5, 'line-style': 'dotted' }
            },
            {
                selector: 'edge[show=0]',
                style: { 'display': 'none' }
            },
            {
                selector: '.eh-handle',
                style: {
                    'background-color': 'red',
                    'width': 6,
                    'height': 6,
                    'shape': 'ellipse',
                    'overlay-opacity': 0,
                    'border-width': 4, // makes the handle easier to hit
                    'border-opacity': 0
                }
            }

        ];

        return netstyle;
    }

    ErrorMsg(hdr, msg) {
        //$divMsg = $('#divMessage').first();
        //$divMsg.addClass("negative").show();
        //
        //$('#divMsgHeader').text(hdr);
        //
        //if (append) {
        //    var html = $('#divMsgBody').html();
        //    $('#divMsgBody').html(html + "<br/>" + msg);
        //}
        //else
        //    $('#divMsgBody').html(msg);

        //let div = this.cy.container().id;
        $('body').toast({ class: 'errorToast', message: msg, title: hdr });
    }
}


// Popup tips
var tip = null;

function MakeTip(ele, text) {
    var ref = ele.popperRef();

    // unfortunately, a dummy element must be passed
    // as tippy only accepts a dom element as the target
    // https://github.com/atomiks/tippyjs/issues/661
    var dummyDomEle = document.createElement('div');

    var _tip = tippy(dummyDomEle, {
        onCreate: function (instance) { // mandatory
            // patch the tippy's popper reference so positioning works
            // https://atomiks.github.io/tippyjs/misc/#custom-position
            instance.popperInstance.reference = ref;
        },
        delay: [1000, 100],
        lazy: false, // mandatory
        //trigger: 'manual', // mandatory - call show() and hide() yourself

        // dom element inside the tippy:
        content: function () { // function can be better for performance
            var div = document.createElement('div');
            div.innerHTML = text;
            return div;
        },
        // your own preferences:
        arrow: true,
        placement: 'bottom',
        hideOnClick: false,
        multiple: true,
        sticky: true,
        allowHTML: true,

        // if interactive:
        interactive: true,
        appendTo: document.body // or append dummyDomEle to document.body
    });

    return _tip;
}

function ShowNodeTip(evt) {
    if (showTips) {
        let node = evt.target;
        let tipText = '<span style="font-size:small">Name: ' + node.data('name')
            + "<br/>Sum Inputs: " + node.data('sumInfs').toPrecision(2)
            + "<br/>Sr Total: " + node.data('srTotal').toPrecision(2)
            + "<br/>Reactivity: " + node.data('reactivity').toPrecision(2)
            + "<br/>Influence: " + node.data('influence').toPrecision(2);

        let snipModel = node.data('snipModel'); 
        let traits = node.data('traits');
        for (var i in traits)
            tipText += "<br/>" + snipModel.networkInfo.traits[i] + ": " + traits[i];

         tipText += "</span>";

        tip = MakeTip(node, tipText);
        tip.show();
    }
}

function HideTip(evt) {
    if (showTips) {
        if (tip !== null)
            tip.hide();

        tip = null;
    }
}

function ShowEdgeTip(evt) {
    if (showTips) {
        var edge = evt.target;
        var tipText = '<span style="font-size:small">Trans Eff: ' + edge.data('transEff').toPrecision(2)
            + "<br/>Trans Time: " + edge.data('transTime').toPrecision(2)
            + "<br/>Signal Strength: " + edge.data('signalStrength').toPrecision(2)
            + "<br/>Influence: " + edge.data('influence').toPrecision(2);


        let snipModel = edge.data('snipModel');
        let traits = edge.data('traits');
        for (var i in traits)
            tipText += "<br/>" + snipModel.networkInfo.traits[i] + ": " + traits[i];

        tipText += "</span>";

        tip = MakeTip(edge, tipText);
        tip.show();
    }
}



function GetNetworkMenuCommands() {
    cmds = [];

    if ($('#divNetworkLegend').is(":visible"))
        cmds.push({ content: 'Hide Network Legend', contentStyle: { "font-size": "small" }, select: () => { $('#divNetworkLegend').hide(); return false; } });
    else
        cmds.push({ content: 'Show Network Legend', contentStyle: { "font-size": "small" }, select: () => { $('#divNetworkLegend').show(); return false; } });

    if ($('#divMotifs').is(":visible"))
        cmds.push({ content: 'Hide Motifs Identifier', contentStyle: { "font-size": "small" }, select: () => { $('#divMotifs').hide(); return false; } });
    else
        cmds.push({ content: 'Show Motifs Identifier', contentStyle: { "font-size": "small" }, select: () => { $('#divMotifs').show(); return false; } });

    cmds.push({ content: 'Zoom Full', contentStyle: { "font-size": "small" }, select: () => network.fit() });

    cmds.push({ content: 'Network Definition', contentStyle: { "font-size": "small" }, select: () => ShowNetworkRep(true) });

    cmds.push({ content: 'Add Node', contentStyle: { "font-size": "small" }, select: () => AddNode() });

    cmds.push({ content: 'Add Edge', contentStyle: { "font-size": "small" }, select: () => StartAddEdge() });

    return cmds;
}

function GetEdgeMenuCommands(edge) {
    cmds = [];

    if (edge.data('state') !== STATE_DELETED)
        cmds.push({
            content: 'Remove', // html/text content to be displayed in the menu
            contentStyle: { "font-size": "small" },
            select: function (ele) { ele.data('state', STATE_DELETED); SolveEqNetwork(0); }
        });
    else
        cmds.push({
            content: 'Restore', // html/text content to be displayed in the menu
            contentStyle: { "font-size": "small" },
            select: function (ele) { ele.data('state', STATE_DELETED); SolveEqNetwork(0); }
        });

    if (edge.data('watch') !== true)
        cmds.push({ // Add to watch
            content: 'Add to Watch List', // html/text content to be displayed in the menu
            contentStyle: { "font-size": "small" },
            select: function (ele) { AddEdgeToWatch(ele); }
        });
    else
        cmds.push({ // Remove fromwatch
            content: 'Remove From Watch List', // html/text content to be displayed in the menu
            contentStyle: { "font-size": "small" },
            select: function (ele) { RemoveEdgeFromWatch(ele); }
        });

    cmds.push({
        content: 'Show Properties', // html/text content to be displayed in the menu
        contentStyle: { "font-size": "small" },
        select: function (ele) { ShowEdgeProperties(ele); }
    });

    /////            { // example command
    /////                //fillColor: 'rgba(200, 200, 200, 0.75)', // optional: custom background color for item
    /////                content: 'Show Node Properties', // html/text content to be displayed in the menu
    /////                contentStyle: { 'color': 'black' }, // css key:value pairs to set the command's css in js if you want
    /////                select: function (ele) { // a function to execute when the command is selected
    /////                    ShowNodeProperties(ele); // `ele` holds the reference to the active element
    /////                },
    /////                enabled: true // whether the command is selectable
    /////            },
    return cmds;
}

function GetNodeMenuCommands(node) {
    cmds = [];

    if (node.data('watch') !== true)
        cmds.push({ // Add to watch
            content: 'Add to Watch List', // html/text content to be displayed in the menu
            contentStyle: { "font-size": "small" },
            select: function (ele) { AddNodeToWatch(ele); }
        });
    else
        cmds.push({ // Remove fromwatch
            content: 'Remove From Watch List', // html/text content to be displayed in the menu
            contentStyle: { "font-size": "small" },
            select: function (ele) { RemoveNodeFromWatch(ele); }
        });

    cmds.push({
        content: 'Show Properties', // html/text content to be displayed in the menu
        contentStyle: { "font-size": "small" },
        select: function (ele) { ShowNodeProperties(ele); }
    });


    cmds.push({
        content: 'Show Neighbors', // html/text content to be displayed in the menu
        contentStyle: { "font-size": "small" },
        select: function (ele) { ShowNodeNeighbors(ele); }
    });

    cmds.push({
        content: 'Remove Node', // html/text content to be displayed in the menu
        contentStyle: { "font-size": "small" },
        select: function (ele) { RemoveNode(ele); }
    });

    if (zoomed) {
        cmds.push({
            content: 'Zoom Out', // html/text content to be displayed in the menu
            contentStyle: { "font-size": "small" },
            select: function (ele) { // a function to execute when the command is selected
                network.nodes().data('opacity', 1);
                network.edges().data('opacity', 1);
                network.fit(); // `ele` holds the reference to the active element
                zoomed = false;
            }
        });
    }
    else {
        cmds.push({
            content: 'Zoom To Neighbors', // html/text content to be displayed in the menu
            contentStyle: { "font-size": "small" },
            select: function (ele) { // a function to execute when the command is selected
                ZoomToNodeNeighbors(ele); // `ele` holds the reference to the active element
                zoomed = true;
            }
        });
    }

    return cmds;
}

