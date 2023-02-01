def discrete_cmap(N, base_cmap=None):
    """
    Create an N-bin discrete colormap from the specified input map
    """
    # Note that if base_cmap is a string or None, you can simply do
    #    return plt.cm.get_cmap(base_cmap, N)
    # The following works for string, None, or a colormap instance:

    base = plt.cm.get_cmap(base_cmap)
    color_list = base(np.linspace(0, 1, N))
    cmap_name = base.name + str(N)
    return base.from_list(cmap_name, color_list, N)

# Code inspired by Google OR Tools plot:
# https://github.com/google/or-tools/blob/fb12c5ded7423d524fc6c95656a9bdc290a81d4d/examples/python/cvrptw_plot.py
def plot_vrptw(ax1, instance, solution=None,
         dist=None, markersize=5, title="VRP", no_legend=False):
    """
    Plot the route(s) on matplotlib axis ax1.
    """
    coords = np.array(instance['coords'])
    demand = np.array(instance['demands'])
    capacity = instance['capacity']
    timew = np.array(instance['time_windows'])
    service_t = instance['service_times']
    dist = instance['duration_matrix']
    min_routes = demand.sum() / capacity
    
    visualize_nodes = True
    
    x_dep, y_dep = coords[0,:]
    ax1.plot(x_dep, y_dep, 'sk', markersize=markersize * 4)
    
    total_dist = 0

    if solution is None:
        if visualize_nodes:
            xs, ys = coords.transpose()
            # color=cmap(0)
            ax1.plot(xs, ys, 'o', mfc='black', markersize=markersize, markeredgewidth=0.0)
        ax1.set_title("{}, min {:.2f} routes".format(title, min_routes) if demand is not None else title)
    else:
        routes = solution

        cmap = discrete_cmap(len(routes) + 2, 'nipy_spectral')
        qvs = []
        total_timewarp = 0
        for veh_number, r in enumerate(routes):
            color = cmap(len(routes) - veh_number)  # Invert to have in rainbow order
            
            route_coords = coords[r, :]
            xs, ys = route_coords.transpose()

            if demand is not None:
                route_demands = demand[r]
                total_route_demand = sum(route_demands)
                assert total_route_demand <= capacity
            if visualize_nodes:
                # Use color of route such that for nodes in an individual route it is clear to which route they belong
                ax1.plot(xs, ys, 'o', mfc=color if len(routes) > 1 else 'black', markersize=markersize, markeredgewidth=0.0)

            r_with_depot = np.concatenate(([0], r))
            route_dist = dist[r_with_depot, np.roll(r_with_depot, -1, 0)].sum()
            total_dist += route_dist

            # Check timewindows 
            prev = 0
            t = timew[0][0]
            timewarp = 0
            for (x, y), n in zip(route_coords, r):
                l, u = timew[n]
                arr = t + service_t[prev] + dist[prev, n]
                t = max(arr, l)
                if t > u:
                    timewarp += t - u
                    t = u
                assert t <= u, f"Time window violated for node {n}: {t} is not in ({l, u})"

                prev = n
            t = t + service_t[prev] + dist[prev, 0]  # Return to depot
            _, u = timew[0]
            if t > u:
                timewarp += t - u
                t = u
            assert t <= u
            
            total_timewarp += timewarp

            # Assume VRP
            label = 'R{}, # {}, c {} / {}, d {:d}{}{}'.format(
                veh_number,
                len(r),
                total_route_demand,
                capacity,
                route_dist,
                ", t {:d}".format(t),
                ", TW {:d}".format(timewarp) if timewarp > 0 else "",
            )

            qv = ax1.quiver(
                xs[:-1],
                ys[:-1],
                xs[1:] - xs[:-1],
                ys[1:] - ys[:-1],
                scale_units='xy',
                angles='xy',
                scale=1,
                color=color,
                label=label,
                width=0.001,
                headlength=20,
                headwidth=12,
            )

            qvs.append(qv)
        title_timewarp = ", TIMEWARP {:d}".format(total_timewarp) if total_timewarp > 0 else ""
        title = '{}, {} routes (min {:.2f}), total distance {:d}{}'.format(title, len(routes), min_routes, total_dist, title_timewarp)
        ax1.set_title(title)
        if label is not None and not no_legend:
            ax1.legend(handles=qvs)

    return total_dist
    

def plot_timew(ax, timewi):
    """Plot number of locations/customers that can be delivered at any time during the day"""
    timewi = np.array(timewi)
    horizon = timewi[0, 1] - timewi[0, 0]
    frm, to = timewi[1:].T

    times = np.concatenate((timewi[0], frm, to))
    deltas = np.concatenate((np.zeros_like(timewi[0]), np.ones_like(frm), -np.ones_like(to)))
    order = np.argsort(times)
    times[order]

    ax.step(times[order], np.cumsum(deltas[order]), where='post')
    ax.set_xlim(timewi[0])

    avg = ((timewi[1:, 1] - timewi[1:, 0]).sum() / horizon).round(1)
    ax.set_xlabel('Time')
    ax.set_ylabel('Num locations')
    ax.set_title(f'Time windows over horizon (avg {avg})')


def plot_demands(ax, demand, capacity):
    """Plot demands CDF by sorting them"""
    ax.plot(sorted(demand))
    avg = np.mean(demand)
    stops_per_route = capacity / avg
    ax.set_title(f'Demands (avg {avg.round(1)}/{capacity} so {stops_per_route.round(1)} stops/route)')
    ax.set_xlabel('Nodes')
    ax.set_ylabel('Demand')
    ax.set_ylim([0, max(demand) + 1])


def plot_schedule(ax, instance, solution=None, markersize=5, title="VRP", tw_linewidth=2, no_legend=False):
    """
    Plot the route(s) schedule on matplotlib axis ax.
    On the x-axis, plot the cumulative distance
    On the y-axis, plot five lines:
    - dotted: cumulative cost/distance/driving time in route
    - dashed: cumulative driving time + service time in route
    - dense: current time: cumulative driving, waiting and service time, resetted by 'time warps'
    - small dash: latest possible time when doing time calculations in reverse starting at the end (possible forward time warps)
    - shading: cumulative demand/load (y-axis normalized to vehicle capacity)
    """
    
    coords = instance['coords']
    demand = instance['demands']
    capacity = instance['capacity']
    timew = instance['time_windows']
    service_t = instance['service_times']
    dist = instance['duration_matrix']
    min_routes = demand.sum() / capacity
    routes = solution
        
    visualize_nodes = True
    
    node_plot_data = []

    cmap = discrete_cmap(len(routes) + 2, 'nipy_spectral')
    qvs = []
    total_dist = 0
    cum_cost = 0
    total_timewarp = 0
    for veh_number, r in enumerate(routes):
        color = cmap(len(routes) - veh_number)  # Invert to have in rainbow order
        
        route_coords = coords[r, :]
        xs, ys = route_coords.transpose()

        route_demands = demand[r]
        total_route_demand = sum(route_demands)
        assert total_route_demand <= capacity

        r_with_depot = np.concatenate(([0], r))
        route_dist = dist[r_with_depot, np.roll(r_with_depot, -1, 0)].sum()
        
        prev = 0
        assert service_t[0] == 0
        horizon = timew[0][1] - timew[0][0]
        t = timew[0][0]
        cum_demand = 0
        route_cost = 0
        cum_service = 0
        timewarp = 0
        route_plot_data = [(cum_cost, t, route_cost + cum_service, cum_demand)]
        for (x, y), n, d in zip(route_coords, r, route_demands):
            l, u = timew[n]
            arr = t + service_t[prev] + dist[prev, n]
            t = max(arr, l)
            cum_cost += dist[prev, n]
            route_cost += dist[prev, n]
            cum_demand += d
            cum_service += service_t[n]
            
            if t > u:
                timewarp += t - u
                print('node', n, 'timewarp', timewarp)
                t = u
            
            assert t <= u, f"Time window violated for node {n}: {t} is not in ({l, u})"
            assert cum_demand <= capacity
            
            # Add timewindow line
            node_plot_data.append((
                [(cum_cost, l), (cum_cost, u)],  # Timewindow
                'k' if (u - l) < horizon / 2 else 'gray', # Timewindow color
                [(cum_cost, t), (cum_cost, t + service_t[n])],  # Service window
                color, # Service window color
                [(cum_cost, arr), (cum_cost, t)] if arr > t else None
            ))
            
            route_plot_data.append((cum_cost, arr, route_cost + cum_service - service_t[n], cum_demand - d))
            route_plot_data.append((cum_cost, t + service_t[n], route_cost + cum_service, cum_demand))
            
            prev = n
        
        t = t + service_t[prev] + dist[prev, 0]  # Return to depot
        cum_cost += dist[prev, 0]
        route_cost += dist[prev, 0]
        _, u = timew[0]
        if t > u:
            timewarp += t - u
            t = u
        assert t <= u
        total_timewarp += timewarp
        
        route_plot_data.append((cum_cost, t, route_cost + cum_service - service_t[0], cum_demand))
        
        xs, ys, ys_exclwait, ys_demand = zip(*route_plot_data)
        ax.plot(xs, ys, color=color)
        ax.plot([cum_cost - route_cost, cum_cost], [0, route_cost], color=color, linestyle=':') # Cumulative cost only (excl service & waiting)
        ax.plot(xs, ys_exclwait, color=color, linestyle='-.')  # Cumulative cost + service time (excl waiting)
        # Scale demand to plot between 0 and 100 %
        ys_demand = np.array(ys_demand) / capacity * horizon
        ax.fill_between(xs, ys_demand, color=color, alpha=0.1)
        
        # Do the same thing in reverse to find last possibility
        t = timew[0][1]
        nxt = 0
        cum_cost_rev = cum_cost
        reverse_timewarp = 0
        route_plot_data = [(cum_cost_rev, t)]
        for (x, y), n, d in reversed(list(zip(route_coords, r, route_demands))):
            l, u = timew[n]
            dep = t - dist[n, nxt] # latest departure
            t = min(dep - service_t[n], u) # latest time to start service = latest arrival
            cum_cost_rev -= dist[n, nxt]
            if t < l:
                reverse_timewarp += l - t
                t = l
            assert t >= l, f"Time window violated for node {n}: {t} is not in ({l, u})"
            
            route_plot_data.append((cum_cost_rev, dep))
            route_plot_data.append((cum_cost_rev, t))
            
            nxt = n
        
        t = t - dist[0, nxt]
        l, _ = timew[0]
        if t < l:
            reverse_timewarp += l - t
            t = l
        assert t >= l
        assert timewarp == reverse_timewarp
        cum_cost_rev -= dist[0, nxt]
        route_plot_data.append((cum_cost_rev, t + service_t[0]))
    
        xs, ys = zip(*route_plot_data)
        ax.plot(xs, ys, color=color, linestyle='--')
        
        total_dist += route_dist
    
    timew = np.array(timew)
    
    timew_lines, timew_colors, node_service_lines, node_service_colors, timewarps = zip(*node_plot_data)
    timewarps = [tw for tw in timewarps if tw is not None]
    
    linecoll_tw = matcoll.LineCollection(timew_lines, colors=timew_colors, linewidths=tw_linewidth)
    linecoll_timewarps = matcoll.LineCollection(timewarps, colors='red', linewidths=tw_linewidth / 2, alpha=0.7) 
    
    ax.add_collection(linecoll_tw)
    ax.add_collection(linecoll_timewarps)
    
    ax.set_xlim([0, total_dist])
    ax.set_ylim(timew[0])
    ax.set_xlabel('Distance')
    ax.set_ylabel('Time')
    ax.set_title(title)